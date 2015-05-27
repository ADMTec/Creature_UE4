// UE4 Procedural Mesh Generation from the Epic Wiki (https://wiki.unrealengine.com/Procedural_Mesh_Generation)
//

#include "ProceduralMesh.h"
#include "CreatureActor.h"

static std::map<std::string, std::shared_ptr<CreatureModule::CreatureAnimation> > global_animations;
static std::map<std::string, std::shared_ptr<CreatureModule::CreatureLoadDataPacket> > global_load_data_packets;

static std::string GetAnimationToken(const std::string& filename_in, const std::string& name_in)
{
	return filename_in + std::string("_") + name_in;
}

static std::string ConvertToString(FString str)
{
	std::string t = TCHAR_TO_UTF8(*str);
	return t;
}

ACreatureActor::ACreatureActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	animation_speed = 1.0f;
	smooth_transitions = false;

	mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CreatureActor"));

	// Apply a simple material directly using the VertexColor as its BaseColor input
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("Material'/Game/Materials/BaseColor.BaseColor'"));
	// TODO Apply a real material with textures, using UVs
//	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("Material'/Game/Materials/M_Concrete_Poured.M_Concrete_Poured'"));
	mesh->SetMaterial(0, Material.Object);

	// Generate a single dummy triangle
	TArray<FProceduralMeshTriangle> triangles;
	GenerateTriangle(triangles);
	mesh->SetProceduralMeshTriangles(triangles);

	RootComponent = mesh;

	// Test Creature code
	/*
	std::string creature_json("C:\\Work\\CreatureDataExport2.CreaExport\\character_data.json");
	ACreatureActor::LoadDataPacket(creature_json);
	ACreatureActor::LoadAnimation(creature_json, "default");

	LoadCreature(creature_json);
	AddLoadedAnimation(creature_json, "default");
	SetActiveAnimation("default");
	creature_manager->Update(0.1f);
	UpdateCreatureRender();
	*/
}

void ACreatureActor::OnConstruction(const FTransform & Transform)
{
	Super::OnConstruction(Transform);
	bool retval = InitCreatureRender();
	if (retval)
	{
		Tick(0.1f);
	}
}

void ACreatureActor::PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	bool retval = InitCreatureRender();
	if (retval)
	{
		Tick(0.1f);
	}

}

bool ACreatureActor::InitCreatureRender()
{
	bool does_exist = FPlatformFileManager::Get().GetPlatformFile().FileExists(*creature_filename);
	if (does_exist)
	{
		auto load_filename = ConvertToString(creature_filename);
		// try to load creature
		ACreatureActor::LoadDataPacket(load_filename);
		LoadCreature(load_filename);

		// try to load all animations
		auto all_animation_names = creature_manager->GetCreature()->GetAnimationNames();
		auto first_animation_name = all_animation_names[0];
		for (auto& cur_name : all_animation_names)
		{
			ACreatureActor::LoadAnimation(load_filename, cur_name);
			AddLoadedAnimation(load_filename, cur_name);
		}

		auto cur_str = ConvertToString(start_animation_name);
		for (auto& cur_name : all_animation_names)
		{
			if (cur_name == cur_str)
			{
				first_animation_name = cur_name;
				break;
			}
		}

		SetActiveAnimation(first_animation_name);

		if (smooth_transitions)
		{
			creature_manager->SetAutoBlending(true);
		}

		return true;
	}
	else {
		UE_LOG(LogTemp, Warning, TEXT("ACreatureActor::BeginPlay() - ERROR! Could not load creature file: %s"), *creature_filename);
	}

	return false;
}

void ACreatureActor::BeginPlay()
{
	InitCreatureRender();
}

void ACreatureActor::LoadDataPacket(const std::string& filename_in)
{
	if (global_load_data_packets.count(filename_in) > 0)
	{
		// file already loaded, just return
		return;
	}

	std::shared_ptr<CreatureModule::CreatureLoadDataPacket> new_packet =
		std::make_shared<CreatureModule::CreatureLoadDataPacket>();
	CreatureModule::LoadCreatureJSONData(filename_in, *new_packet);

	global_load_data_packets[filename_in] = new_packet;
}

void ACreatureActor::LoadAnimation(const std::string& filename_in, const std::string& name_in)
{
	auto cur_token = GetAnimationToken(filename_in, name_in);
	if (global_animations.count(cur_token) > 0)
	{
		// animation already exists, just return
		return;
	}

	auto load_data = global_load_data_packets[filename_in];

	std::shared_ptr<CreatureModule::CreatureAnimation> new_animation =
		std::make_shared<CreatureModule::CreatureAnimation>(*load_data, name_in);

	global_animations[cur_token] = new_animation;
}

void ACreatureActor::LoadCreature(const std::string& filename_in)
{
	auto load_data = global_load_data_packets[filename_in];

	std::shared_ptr<CreatureModule::Creature> new_creature =
		std::make_shared<CreatureModule::Creature>(*load_data);

	creature_manager = std::make_shared<CreatureModule::CreatureManager>(new_creature);

	draw_triangles.SetNum(creature_manager->GetCreature()->GetTotalNumIndices() / 3, true);
	mesh->SetProceduralMeshTriangles(draw_triangles);
}

bool ACreatureActor::AddLoadedAnimation(const std::string& filename_in, const std::string& name_in)
{
	auto cur_token = GetAnimationToken(filename_in, name_in);
	if (global_animations.count(cur_token) > 0)
	{
		creature_manager->AddAnimation(global_animations[cur_token]);
		creature_manager->SetIsPlaying(true);
		creature_manager->SetShouldLoop(true);
		return true;
	}
	else {
		std::cout << "ERROR! ACreatureActor::AddLoadedAnimation() Animation with filename: " << filename_in << " and name: " << name_in << " not loaded!" << std::endl;
	}

	return false;
}

void ACreatureActor::SetBluePrintActiveAnimation(FString name_in)
{
	auto cur_str = ConvertToString(name_in);
	SetActiveAnimation(cur_str);
}

void ACreatureActor::SetActiveAnimation(const std::string& name_in)
{
	creature_manager->SetActiveAnimationName(name_in);
}

void ACreatureActor::SetBluePrintBlendActiveAnimation(FString name_in, float factor)
{
	auto cur_str = ConvertToString(name_in);
	SetAutoBlendActiveAnimation(cur_str, factor);
}

void 
ACreatureActor::SetAutoBlendActiveAnimation(const std::string& name_in, float factor)
{
	if (smooth_transitions == false)
	{
		smooth_transitions = true;
		creature_manager->SetAutoBlending(true);
	}

	creature_manager->AutoBlendTo(name_in, factor);
}

void ACreatureActor::Tick(float DeltaTime)
{
	if (creature_manager)
	{
		creature_manager->Update(DeltaTime * animation_speed);
		UpdateCreatureRender();
	}
}

void ACreatureActor::UpdateCreatureRender()
{
	auto cur_creature = creature_manager->GetCreature();
	int num_triangles = cur_creature->GetTotalNumIndices() / 3;
	glm::uint32 * cur_idx = cur_creature->GetGlobalIndices();
	glm::float32 * cur_pts = cur_creature->GetRenderPts();
	glm::float32 * cur_uvs = cur_creature->GetGlobalUvs();

	TArray<FProceduralMeshTriangle>& write_triangles = mesh->GetProceduralTriangles();

	static const FColor White(255, 255, 255, 255);
	int cur_pt_idx = 0, cur_uv_idx = 0;

	for (int i = 0; i < num_triangles; i++)
	{
		int real_idx_1 = cur_idx[0];
		int real_idx_2 = cur_idx[1];
		int real_idx_3 = cur_idx[2];

		FProceduralMeshTriangle triangle;

		cur_pt_idx = real_idx_1 * 3;
		cur_uv_idx = real_idx_1 * 2;
		triangle.Vertex0.Position.Set(cur_pts[cur_pt_idx], cur_pts[cur_pt_idx + 1], cur_pts[cur_pt_idx + 2]);
		triangle.Vertex0.Color = White;
		triangle.Vertex0.U = cur_uvs[cur_uv_idx];
		triangle.Vertex0.V = cur_uvs[cur_uv_idx + 1];

		cur_pt_idx = real_idx_2 * 3;
		cur_uv_idx = real_idx_2 * 2;
		triangle.Vertex1.Position.Set(cur_pts[cur_pt_idx], cur_pts[cur_pt_idx + 1], cur_pts[cur_pt_idx + 2]);
		triangle.Vertex1.Color = White;
		triangle.Vertex1.U = cur_uvs[cur_uv_idx];
		triangle.Vertex1.V = cur_uvs[cur_uv_idx + 1];

		cur_pt_idx = real_idx_3 * 3;
		cur_uv_idx = real_idx_3 * 2;
		triangle.Vertex2.Position.Set(cur_pts[cur_pt_idx], cur_pts[cur_pt_idx + 1], cur_pts[cur_pt_idx + 2]);
		triangle.Vertex2.Color = White;
		triangle.Vertex2.U = cur_uvs[cur_uv_idx];
		triangle.Vertex2.V = cur_uvs[cur_uv_idx + 1];

		write_triangles[i] = triangle;

		cur_idx += 3;
	}

	//mesh->SetProceduralMeshTriangles(draw_triangles);
	mesh->ForceAnUpdate();
}

// Generate a single horizontal triangle counterclockwise to point up (one face, visible only from the top, not from the bottom)
void ACreatureActor::GenerateTriangle(TArray<FProceduralMeshTriangle>& OutTriangles)
{
	FProceduralMeshTriangle triangle;
	triangle.Vertex0.Position.Set(  0.f, -10.f, 0.f);
	triangle.Vertex1.Position.Set(  0.f,  10.f, 0.f);
	triangle.Vertex2.Position.Set(10.f,  0.f,  0.f);
	static const FColor Blue(51, 51, 255);
	triangle.Vertex0.Color = Blue;
	triangle.Vertex1.Color = Blue;
	triangle.Vertex2.Color = Blue;
	triangle.Vertex0.U = 0.0f;
	triangle.Vertex0.V = 0.0f;
	triangle.Vertex1.U = 1.0f;
	triangle.Vertex1.V = 0.0f;
	triangle.Vertex2.U = 0.5f;
	triangle.Vertex2.V = 0.75f;
	OutTriangles.Add(triangle);
}
