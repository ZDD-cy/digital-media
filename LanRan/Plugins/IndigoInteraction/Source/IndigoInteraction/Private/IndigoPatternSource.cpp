#include "IndigoPatternSource.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"

AIndigoPatternSource::AIndigoPatternSource()
{
    Kind = EIndigoInteractionKind::DragSource;
    bReturnToSource = true;
    BoardMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoardMesh"));
    BoardMesh->SetupAttachment(HitBox);
    BoardMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PatternLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("PatternLabel"));
    PatternLabel->SetupAttachment(HitBox);
    PatternLabel->SetHorizontalAlignment(EHTA_Center);
    PatternLabel->SetVerticalAlignment(EVRTA_TextCenter);
    PatternLabel->SetWorldSize(18.f);
    PatternLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AIndigoPatternSource::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (UStaticMesh* Mesh = BoardMesh->GetStaticMesh())
    {
        const FBoxSphereBounds Bounds = Mesh->GetBounds();
        BoardMesh->SetRelativeScale3D(FVector(VisualScale));
        BoardMesh->SetRelativeLocation(-Bounds.Origin * VisualScale);
        FVector Extent = Bounds.BoxExtent * VisualScale + FVector(2.f);
        Extent.Z = FMath::Max(6.f, Extent.Z);
        HitBox->SetBoxExtent(Extent);
        PatternLabel->SetRelativeLocation(FVector(0, 0, Extent.Z + 13.f));
    }
    const TCHAR* Label = Pattern == EIndigoPattern::A ? TEXT("A") : Pattern == EIndigoPattern::B ? TEXT("B") : Pattern == EIndigoPattern::C ? TEXT("C") : TEXT("?");
    PatternLabel->SetText(FText::FromString(Label));
    PatternLabel->SetTextRenderColor(Pattern == EIndigoPattern::A ? FColor(80,190,255) : Pattern == EIndigoPattern::B ? FColor(100,255,160) : FColor(255,210,80));
}
