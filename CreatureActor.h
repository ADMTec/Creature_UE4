// UE4 Procedural Mesh Generation from the Epic Wiki (https://wiki.unrealengine.com/Procedural_Mesh_Generation)

#pragma once

#include "GameFramework/Actor.h"

#include "CreatureModule.h"
#include <map>

#include "CustomProceduralMeshComponent.h"
#include "CreatureActor.generated.h"

/**
 * 
 */

USTRUCT()
struct FCreatureBoneData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Materials)
	FVector point1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Materials)
	FVector point2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Materials)
	FString name;
};

UCLASS(Blueprintable)
class ACreatureActor : public AActor
{
	GENERATED_BODY()

protected:
	TArray<FProceduralMeshTriangle> draw_triangles;

	std::shared_ptr<CreatureModule::CreatureManager> creature_manager;

	TArray<FCreatureBoneData> bone_data;


	void UpdateCreatureRender();

	bool InitCreatureRender();

	void FillBoneData();

public:
	ACreatureActor();

	// Allow viewing/changing the Material ot the procedural Mesh in editor (if placed in a level at construction)
	UPROPERTY(VisibleAnywhere, Category=Materials)
	UCustomProceduralMeshComponent* mesh;

	UPROPERTY(VisibleAnywhere, Category = Collision)
	UCapsuleComponent * rootCollider;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Materials)
	FString creature_filename;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Materials)
	float animation_speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Materials)
	bool smooth_transitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Materials)
	FString start_animation_name;


	virtual void PostEditChangeProperty(FPropertyChangedEvent & PropertyChangedEvent);

	virtual void OnConstruction(const FTransform & Transform);

	// Loads a data packet from a file
	static void LoadDataPacket(const std::string& filename_in);

	// Loads an animation from a file
	static void LoadAnimation(const std::string& filename_in, const std::string& name_in);

	// Loads the creature character from a file
	void LoadCreature(const std::string& filename_in);

	// Adds a loaded animation onto the creature character
	bool AddLoadedAnimation(const std::string& filename_in, const std::string& name_in);

	// Blueprint version of setting the active animation name
	UFUNCTION(BlueprintCallable, Category = "Components|Creature")
	void SetBluePrintActiveAnimation(FString name_in);

	// Blueprint version of setting the blended active animation name
	UFUNCTION(BlueprintCallable, Category = "Components|Creature")
	void SetBluePrintBlendActiveAnimation(FString name_in, float factor);

	// Blueprint version of setting a custom time range for a given animation
	UFUNCTION(BlueprintCallable, Category = "Components|Creature")
	void SetBluePrintAnimationCustomTimeRange(FString name_in, int32 start_time, int32 end_time);

	// Blueprint function that returns the bone data given a bone name
	UFUNCTION(BlueprintCallable, Category = "Components|Creature")
	FCreatureBoneData GetBluePrintBoneData(FString name_in, bool world_transform);

	// Sets the an active animation by name
	void SetActiveAnimation(const std::string& name_in);

	// Sets the active animation by smoothly blending, factor is a range of ( 0 < factor < 1 )
	void SetAutoBlendActiveAnimation(const std::string& name_in, float factor);

	

	// Update callback
	virtual void Tick(float DeltaTime) override;

	// Called on startup
	virtual void BeginPlay();

	void GenerateTriangle(TArray<FProceduralMeshTriangle>& OutTriangles);
};
