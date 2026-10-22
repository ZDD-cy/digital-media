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
        BoardMesh->SetRelativeLocation(-BoardMesh->GetRelativeRotation().RotateVector(Bounds.Origin * VisualScale));
        const FVector E=Bounds.BoxExtent*VisualScale;
        const FRotationMatrix Rotation(BoardMesh->GetRelativeRotation());
        FVector Extent=Rotation.GetScaledAxis(EAxis::X).GetAbs()*E.X+Rotation.GetScaledAxis(EAxis::Y).GetAbs()*E.Y+Rotation.GetScaledAxis(EAxis::Z).GetAbs()*E.Z+FVector(2.f);
        Extent.Z = FMath::Max(6.f, Extent.Z);
        HitBox->SetBoxExtent(Extent);
        PatternLabel->SetRelativeLocation(FVector(0, 0, Extent.Z + 13.f));
    }
    const TCHAR* Label = Pattern == EIndigoPattern::A ? TEXT("A") : Pattern == EIndigoPattern::B ? TEXT("B") : Pattern == EIndigoPattern::C ? TEXT("C") : TEXT("?");
    PatternLabel->SetText(FText::FromString(Label));
    PatternLabel->SetTextRenderColor(Pattern == EIndigoPattern::A ? FColor(80,190,255) : Pattern == EIndigoPattern::B ? FColor(100,255,160) : FColor(255,210,80));
}
