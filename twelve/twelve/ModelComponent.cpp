#include "ModelComponent.h"
#include "Renderer.h"
#include "Game.h"
#include "Actor.h"
#include "Scene.h"
#include "Helper.h"
#include "XMFLOAT_Helper.h"
#include <d3dx12.h>
#include <d3d12.h>

using namespace DirectX;

ModelComponent::ModelComponent(Actor* owner, ModelType type, const char* file_name, int draw_order) :
	Component(owner, draw_order)
{
	dx12_ = owner_->GetScene()->GetGame()->GetDx12();
	renderer_ = owner_->GetScene()->GetGame()->GetRenderer();

	renderer_->AddModelComponent(this, type);

	// ���f��
	switch (type)
	{
		case PMD:
			pmd_model_ = dx12_->LoadPMDModel(file_name);
			bone_matrices_.resize(pmd_model_->bone_name_array.size());
			std::fill(bone_matrices_.begin(), bone_matrices_.end(), DirectX::XMMatrixIdentity());

			break;
		case FBX:
			fbx_model_ = dx12_->LoadFBXModel(file_name);

			break;
		default:
			break;
	}


	// ���W�ϊ��p���\�[�X�쐬
	auto result = CreateTransformResourceAndView();

	if (FAILED(result))
	{
		assert(0);
	}
}

ModelComponent::~ModelComponent()
{
	renderer_->RemoveModelComponent(this);
}

void ModelComponent::Update(float delta_time)
{
	// ���[���h�s��X�V
	DirectX::XMFLOAT3 pos = owner_->GetPosition();
	DirectX::XMFLOAT3 rot = owner_->GetRotation();

	//angle_ += 0.1f;

	mapped_matrices_[0] = DirectX::XMMatrixRotationY(angle_) * DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);

	MotionUpdate(delta_time);
}

void ModelComponent::DrawPMD()
{
	auto cmd_list = dx12_->GetCommandList();
	cmd_list->IASetVertexBuffers(0, 1, &pmd_model_->vb_view);
	cmd_list->IASetIndexBuffer(&pmd_model_->ib_view);
	cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	ID3D12DescriptorHeap* trans_heaps[] = { transform_heap_.Get() };
	cmd_list->SetDescriptorHeaps(1, trans_heaps);
	cmd_list->SetGraphicsRootDescriptorTable(1, transform_heap_->GetGPUDescriptorHandleForHeapStart());

	auto material_heap = pmd_model_->material_heap.Get();
	ID3D12DescriptorHeap* material_heaps[] = { material_heap };
	cmd_list->SetDescriptorHeaps(1, material_heaps);

	// �}�e���A���̕`��
	auto handle = material_heap->GetGPUDescriptorHandleForHeapStart();
	unsigned int idxOffset = 0;

	auto inc_size = dx12_->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV) * 5;
	for (auto& material : pmd_model_->materials)
	{
		cmd_list->SetGraphicsRootDescriptorTable(2, handle);
		cmd_list->DrawIndexedInstanced(material.indices_num, 2, idxOffset, 0, 0);
		handle.ptr += inc_size;
		idxOffset += material.indices_num;
	}
}

void ModelComponent::DrawFBX()
{
	for (auto& indices : fbx_model_->index_table)
	{
		auto& node_name = indices.first;

		auto cmd_list = dx12_->GetCommandList();
		cmd_list->IASetVertexBuffers(0, 1, &fbx_model_->vb_view_table[node_name]);
		cmd_list->IASetIndexBuffer(&fbx_model_->ib_view_table[node_name]);
		cmd_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		cmd_list->DrawIndexedInstanced(indices.second.size(), 1, 0, 0, 0);
	}
}

unsigned int ModelComponent::AddAnimation(const char* file_name, bool is_loop)
{
	unsigned int idx = animation_idx_;
	animation_idx_++;

	Animation animation;
	animation.vmd_anim = dx12_->LoadVMDAnimation(file_name);
	animation.is_loop = is_loop;

	animations_[idx] = animation;

	return idx;
}

void ModelComponent::DeleteAnimation(unsigned int idx)
{
	animations_.erase(idx);
}

HRESULT ModelComponent::CreateTransformResourceAndView()
{
	// GPU�o�b�t�@�̍쐬
	auto buffer_size = sizeof(DirectX::XMMATRIX) * (1 + bone_matrices_.size()); // wordl�s�� + �{�[���s�� * �{�[����
	buffer_size = AligmentedValue(buffer_size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT); // 256�̔{���ɂ���

	auto heap_prop = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	auto res_desc = CD3DX12_RESOURCE_DESC::Buffer(buffer_size);

	auto result = dx12_->GetDevice()->CreateCommittedResource(
		&heap_prop,
		D3D12_HEAP_FLAG_NONE,
		&res_desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(transform_buffer_.ReleaseAndGetAddressOf())
	);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// �}�b�v
	result = transform_buffer_->Map(0, nullptr, (void**)&mapped_matrices_);

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	// ���[���h�s��
	mapped_matrices_[0] = DirectX::XMMatrixIdentity();
	// �{�[���s��
	std::copy(bone_matrices_.begin(), bone_matrices_.end(), mapped_matrices_ + 1);

	// �r���[�̍쐬
	D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
	heap_desc.NumDescriptors = 1;
	heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heap_desc.NodeMask = 0;
	heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	result = dx12_->GetDevice()->CreateDescriptorHeap(&heap_desc, IID_PPV_ARGS(transform_heap_.ReleaseAndGetAddressOf()));

	if (FAILED(result))
	{
		assert(0);
		return result;
	}

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
	cbv_desc.BufferLocation = transform_buffer_->GetGPUVirtualAddress();
	cbv_desc.SizeInBytes = buffer_size;

	dx12_->GetDevice()->CreateConstantBufferView(&cbv_desc, transform_heap_->GetCPUDescriptorHandleForHeapStart());

	return S_OK;
}

void ModelComponent::MotionUpdate(float delta_time)
{
	animation_time_ += delta_time;
	unsigned int frame_no = 30 * animation_time_;

	auto& current_anim = animations_[current_animation_idx_];

	if (frame_no > current_anim.vmd_anim->duration)
	{
		animation_time_ = 0;
	}

	// �s��̃N���A
	std::fill(bone_matrices_.begin(), bone_matrices_.end(), DirectX::XMMatrixIdentity());

	auto& animation = current_anim.vmd_anim;

	// ���[�V�����̍X�V
	for (auto& motion : animation->motion_table)
	{
		auto& bone_name = motion.first;
		auto it_bone_node = pmd_model_->bone_node_table.find(bone_name);

		if (it_bone_node == pmd_model_->bone_node_table.end())
		{
			continue;
		}

		auto& node = it_bone_node->second;
		auto& motions = motion.second;

		auto rit = std::find_if(motions.rbegin(), motions.rend(), [frame_no](const KeyFrame& key_frame)
								{
									return key_frame.frame_no <= frame_no;
								});

		if (rit == motions.rend())
		{
			continue;
		}

		// ���
		DirectX::XMMATRIX rotation = DirectX::XMMatrixIdentity();
		DirectX::XMVECTOR offset = DirectX::XMLoadFloat3(&rit->offset);

		auto it = rit.base();

		if (it != motions.end())
		{
			auto nextFrame = it->frame_no;
			auto prevFrame = rit->frame_no;
			auto t = static_cast<float>(frame_no - prevFrame) / static_cast<float>(nextFrame - prevFrame); // ���ۂɂ� x
			t = GetYFromXOnBezier(t, it->p1, it->p2, 12);

			rotation = DirectX::XMMatrixRotationQuaternion(DirectX::XMQuaternionSlerp(rit->quaternion, it->quaternion, t));
			offset = DirectX::XMVectorLerp(offset, DirectX::XMLoadFloat3(&it->offset), t);
		}
		else
		{
			rotation = DirectX::XMMatrixRotationQuaternion(rit->quaternion);
		}

		auto& pos = node.start_pos;
		auto mat = DirectX::XMMatrixTranslation(-pos.x, -pos.y, -pos.z)
			* rotation
			* DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
		bone_matrices_[node.bone_idx] = mat * DirectX::XMMatrixTranslationFromVector(offset);
	}

	//RecursiveMatrixMultipy(&mBoneNodeTable["�S�Ă̐e"], DirectX::XMMatrixIdentity());
	RecursiveMatrixMultipy(&pmd_model_->bone_node_table["�Z���^�["], DirectX::XMMatrixIdentity());

	IKSolve(frame_no);

	std::copy(bone_matrices_.begin(), bone_matrices_.end(), mapped_matrices_ + 1);
}

void ModelComponent::RecursiveMatrixMultipy(BoneNode* node, const DirectX::XMMATRIX& mat)
{
	bone_matrices_[node->bone_idx] *= mat;
	for (auto& child : node->children)
	{
		RecursiveMatrixMultipy(child, bone_matrices_[node->bone_idx]);
	}
}

void ModelComponent::SolveCCDIK(const PMDIK& ik)
{
	//�덷�͈͓̔����ǂ����Ɏg�p����萔
	constexpr float epsilon = 0.0005f;

	//�^�[�Q�b�g
	auto target_node = pmd_model_->bone_node_address_array[ik.bone_idx];
	auto target_origin_pos = DirectX::XMLoadFloat3(&target_node->start_pos);

	auto parent_mat = bone_matrices_[target_node->ik_parent_bone];
	DirectX::XMVECTOR det;
	auto inv_parent_mat = DirectX::XMMatrixInverse(&det, parent_mat);
	auto target_next_pos = DirectX::XMVector3Transform(target_origin_pos, bone_matrices_[ik.bone_idx] * inv_parent_mat);


	// �܂���IK�̊Ԃɂ���{�[���̍��W�����Ă���(�t������)
	std::vector<DirectX::XMVECTOR> bone_positions;
	// ���[�m�[�h
	auto end_pos = DirectX::XMLoadFloat3(&pmd_model_->bone_node_address_array[ik.target_bone_idx]->start_pos);
	// ���ԃm�[�h(���[�g���܂�)
	for (auto& cidx : ik.node_idxes)
	{
		bone_positions.push_back(DirectX::XMLoadFloat3(&pmd_model_->bone_node_address_array[cidx]->start_pos));
	}

	std::vector<DirectX::XMMATRIX> mats(bone_positions.size());
	fill(mats.begin(), mats.end(), DirectX::XMMatrixIdentity());

	auto ik_limit = ik.limit * DirectX::XM_PI;
	// ik�ɐݒ肳��Ă��鎎�s�񐔂����J��Ԃ�
	for (int c = 0; c < ik.iterations; ++c)
	{
		// �^�[�Q�b�g�Ɩ��[���قڈ�v�����甲����
		if (DirectX::XMVector3Length(DirectX::XMVectorSubtract(end_pos, target_next_pos)).m128_f32[0] <= epsilon)
		{
			break;
		}
		// ���ꂼ��̃{�[����k��Ȃ���p�x�����Ɉ����|����Ȃ��悤�ɋȂ��Ă���
		for (int bidx = 0; bidx < bone_positions.size(); ++bidx)
		{
			const auto& pos = bone_positions[bidx];

			// ���݂̃m�[�h���疖�[�܂łƁA���݂̃m�[�h����^�[�Q�b�g�܂ł̃x�N�g�������
			auto vec_to_end = DirectX::XMVectorSubtract(end_pos, pos);
			auto vec_to_target = DirectX::XMVectorSubtract(target_next_pos, pos);
			vec_to_end = DirectX::XMVector3Normalize(vec_to_end);
			vec_to_target = DirectX::XMVector3Normalize(vec_to_target);

			// �قړ����x�N�g���ɂȂ��Ă��܂����ꍇ�͊O�ςł��Ȃ����ߎ��̃{�[���Ɉ����n��
			if (DirectX::XMVector3Length(DirectX::XMVectorSubtract(vec_to_end, vec_to_target)).m128_f32[0] <= epsilon)
			{
				continue;
			}

			// �O�όv�Z����ъp�x�v�Z
			auto cross = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(vec_to_end, vec_to_target));
			float angle = DirectX::XMVector3AngleBetweenVectors(vec_to_end, vec_to_target).m128_f32[0];
			angle = min(angle, ik_limit); // ��]���E�␳
			DirectX::XMMATRIX rot = DirectX::XMMatrixRotationAxis(cross, angle); // ��]�s��
			// pos�𒆐S�ɉ�]
			auto mat = DirectX::XMMatrixTranslationFromVector(-pos)
				* rot
				* DirectX::XMMatrixTranslationFromVector(pos);
			mats[bidx] *= mat; // ��]�s���ێ����Ă���(��Z�ŉ�]�d�ˊ|��������Ă���)
			// �ΏۂƂȂ�_�����ׂĉ�]������(���݂̓_���猩�Ė��[������])
			for (auto idx = bidx - 1; idx >= 0; --idx)
			{
				//��������]������K�v�͂Ȃ�
				bone_positions[idx] = DirectX::XMVector3Transform(bone_positions[idx], mat);
			}
			end_pos = DirectX::XMVector3Transform(end_pos, mat);
			//���������ɋ߂��Ȃ��Ă��烋�[�v�𔲂���
			if (DirectX::XMVector3Length(DirectX::XMVectorSubtract(end_pos, target_next_pos)).m128_f32[0] <= epsilon)
			{
				break;
			}
		}
	}

	int idx = 0;
	for (auto& cidx : ik.node_idxes)
	{
		bone_matrices_[cidx] = mats[idx];
		++idx;
	}
	auto node = pmd_model_->bone_node_address_array[ik.node_idxes.back()];
	RecursiveMatrixMultipy(node, parent_mat);
}

void ModelComponent::SolveCosineIK(const PMDIK& ik)
{
	std::vector<DirectX::XMVECTOR> positions;
	std::array<float, 2> edge_lens;

	auto& target_node = pmd_model_->bone_node_address_array[ik.bone_idx];
	auto target_pos = DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&target_node->start_pos), bone_matrices_[ik.bone_idx]);

	auto end_node = pmd_model_->bone_node_address_array[ik.target_bone_idx];
	positions.emplace_back(DirectX::XMLoadFloat3(&end_node->start_pos));

	for (auto& chainBoneIdx : ik.node_idxes)
	{
		auto bone_node = pmd_model_->bone_node_address_array[chainBoneIdx];
		positions.emplace_back(DirectX::XMLoadFloat3(&bone_node->start_pos));
	}

	std::reverse(positions.begin(), positions.end());

	edge_lens[0] = DirectX::XMVector3Length(DirectX::XMVectorSubtract(positions[1], positions[0])).m128_f32[0];
	edge_lens[1] = DirectX::XMVector3Length(DirectX::XMVectorSubtract(positions[2], positions[1])).m128_f32[0];

	positions[0] = DirectX::XMVector3Transform(positions[0], bone_matrices_[ik.node_idxes[1]]);
	positions[2] = DirectX::XMVector3Transform(positions[2], bone_matrices_[ik.bone_idx]);

	// ���[�g�����[�ւ̃x�N�g��������Ă���
	auto linearVec = DirectX::XMVectorSubtract(positions[2], positions[0]);
	float A = DirectX::XMVector3Length(linearVec).m128_f32[0];
	float B = edge_lens[0];
	float C = edge_lens[1];

	linearVec = DirectX::XMVector3Normalize(linearVec);

	// ���[�g����^�񒆂ւ̊p�x�v�Z
	float theta1 = acosf((A * A + B * B - C * C) / (2 * A * B));

	// �^�񒆂���^�[�Q�b�g�ւ̊p�x�v�Z
	float theta2 = acosf((B * B + C * C - A * A) / (2 * B * C));

	// �������߂�
	// �����^�񒆂��u�Ђ��v�ł������ꍇ�ɂ͋����I��X���Ƃ���B
	DirectX::XMVECTOR axis;
	if (std::find(pmd_model_->knee_idxes.begin(), pmd_model_->knee_idxes.end(), ik.node_idxes[0]) == pmd_model_->knee_idxes.end())
	{
		auto vm = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(positions[2], positions[0]));
		auto vt = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(target_pos, positions[0]));
		axis = DirectX::XMVector3Cross(vt, vm);
	}
	else
	{
		auto right = DirectX::XMFLOAT3(1, 0, 0);
		axis = DirectX::XMLoadFloat3(&right);
	}

	//���ӓ_�cIK�`�F�[���͍������Ɍ������Ă��琔�����邽��1���������ɋ߂�
	auto mat1 = DirectX::XMMatrixTranslationFromVector(-positions[0]);
	mat1 *= DirectX::XMMatrixRotationAxis(axis, theta1);
	mat1 *= DirectX::XMMatrixTranslationFromVector(positions[0]);


	auto mat2 = DirectX::XMMatrixTranslationFromVector(-positions[1]);
	mat2 *= DirectX::XMMatrixRotationAxis(axis, theta2 - XM_PI);
	mat2 *= DirectX::XMMatrixTranslationFromVector(positions[1]);

	bone_matrices_[ik.node_idxes[1]] *= mat1;
	bone_matrices_[ik.node_idxes[0]] = mat2 * bone_matrices_[ik.node_idxes[1]];
	bone_matrices_[ik.target_bone_idx] = bone_matrices_[ik.node_idxes[0]];
}

void ModelComponent::SolveLookAt(const PMDIK& ik)
{
	auto root_node = pmd_model_->bone_node_address_array[ik.node_idxes[0]];
	auto targetNode = pmd_model_->bone_node_address_array[ik.target_bone_idx];

	auto root_pos1 = DirectX::XMLoadFloat3(&root_node->start_pos);
	auto target_pos1 = DirectX::XMLoadFloat3(&targetNode->start_pos);

	auto root_pos2 = DirectX::XMVector3Transform(root_pos1, bone_matrices_[ik.node_idxes[0]]);
	auto target_pos2 = DirectX::XMVector3Transform(target_pos1, bone_matrices_[ik.bone_idx]);

	auto origin_vec = DirectX::XMVectorSubtract(target_pos1, root_pos1);
	auto target_vec = DirectX::XMVectorSubtract(target_pos2, root_pos2);

	origin_vec = DirectX::XMVector3Normalize(origin_vec);
	target_vec = DirectX::XMVector3Normalize(target_vec);

	DirectX::XMMATRIX mat = DirectX::XMMatrixTranslationFromVector(-root_pos2)
		* LookAtMatrix(origin_vec, target_vec, DirectX::XMFLOAT3(0, 1, 0), DirectX::XMFLOAT3(1, 0, 0))
		* DirectX::XMMatrixTranslationFromVector(root_pos2);

	bone_matrices_[ik.node_idxes[0]] = mat;
}

void ModelComponent::IKSolve(int frame_no)
{
	auto& anim = animations_[current_animation_idx_].vmd_anim;
	auto it = std::find_if(anim->ik_enable_table.rbegin(), anim->ik_enable_table.rend(),
						   [frame_no](const VMDIKEnable& ikenable)
						   {
							   return ikenable.frame_no <= frame_no;
						   });

	for (auto& ik : pmd_model_->ik_data)
	{
		if (it != anim->ik_enable_table.rend())
		{
			auto ik_enable_it = it->ik_enable_table.find(pmd_model_->bone_name_array[ik.bone_idx]);
			if (ik_enable_it != it->ik_enable_table.end())
			{
				if (!ik_enable_it->second)
				{
					continue;
				}
			}
		}

		auto children_node_count = ik.node_idxes.size();

		switch (children_node_count)
		{
			case 0:
				assert(0);
				continue;
			case 1:
				SolveLookAt(ik);
				break;
			case 2:
				SolveCosineIK(ik);
				break;
			default:
				SolveCCDIK(ik);
		}
	}
}
