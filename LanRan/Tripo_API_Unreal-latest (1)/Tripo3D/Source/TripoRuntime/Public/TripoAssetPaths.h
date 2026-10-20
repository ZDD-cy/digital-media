#pragma once

#include "CoreMinimal.h"
#include "Misc/Paths.h"

/**
 * Single source of truth for the folders this plugin writes to.
 *
 * AssetImportRoot mirrors TripoProtocol::ASSET_IMPORT_ROOT in the
 * Tripo3D-UE-Bridge plugin so both plugins deliver models to the same place in
 * the Content Browser. Keep the two values in sync when either one moves.
 *
 * The staging root is deliberately NOT shared with the bridge. That plugin
 * stages under Intermediate/ and deletes on success; this one keeps the
 * downloaded FBX so the source file stays available after import.
 */
namespace TripoAssetPaths
{
	inline const FString AssetImportRoot = TEXT("/Game/TripoModels");

	// Content-relative twin of AssetImportRoot, for on-disk existence checks.
	inline const FString AssetImportRootRelative = TEXT("TripoModels");

	inline const FString StagingDirName = TEXT("TripoDownloads");

	inline FString StagingRoot()
	{
		return FPaths::ProjectSavedDir() / StagingDirName;
	}

	inline FString AssetImportRootOnDisk()
	{
		return FPaths::ProjectContentDir() / AssetImportRootRelative;
	}
}
