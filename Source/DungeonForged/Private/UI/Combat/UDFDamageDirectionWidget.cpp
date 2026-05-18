// Source/DungeonForged/Private/UI/Combat/UDFDamageDirectionWidget.cpp
#include "UI/Combat/UDFDamageDirectionWidget.h"
#include "Components/Image.h"
#include "GameFramework/Pawn.h"

void UDFDamageDirectionWidget::PulseFromWorldLocation(const FVector& DamageSourceWorldLocation, const float Intensity)
{
	APawn* const Pawn = GetOwningPlayerPawn();
	if (!Pawn)
	{
		return;
	}
	const FVector ToHit = (DamageSourceWorldLocation - Pawn->GetActorLocation()).GetSafeNormal2D();
	if (ToHit.IsNearlyZero())
	{
		return;
	}
	const FVector Fwd = Pawn->GetActorForwardVector().GetSafeNormal2D();
	const FVector Right = Pawn->GetActorRightVector().GetSafeNormal2D();
	const float Dot = FVector::DotProduct(Fwd, ToHit);
	const float Side = FVector::DotProduct(Right, ToHit);

	UImage* Target = nullptr;
	if (FMath::Abs(Dot) > FMath::Abs(Side))
	{
		Target = Dot > 0.f ? Indicator_Top.Get() : Indicator_Bottom.Get();
	}
	else
	{
		Target = Side > 0.f ? Indicator_Right.Get() : Indicator_Left.Get();
	}
	PulseIndicator(Target, FMath::Clamp(Intensity, 0.05f, 1.f));
}

void UDFDamageDirectionWidget::PulseIndicator(UImage* const Target, const float Intensity)
{
	if (!Target)
	{
		return;
	}
	ClearPulse();
	ActiveIndicator = Target;
	PulseElapsed = 0.f;
	PulseTotalDuration = PulseFadeInDuration + PulseFadeOutDuration;
	PulsePeakOpacity = FMath::Clamp(Intensity, 0.15f, 1.f);
	Target->SetRenderOpacity(PulsePeakOpacity);
	Target->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UDFDamageDirectionWidget::ClearPulse()
{
	if (UImage* const Prev = ActiveIndicator.Get())
	{
		Prev->SetRenderOpacity(0.f);
		Prev->SetVisibility(ESlateVisibility::Collapsed);
	}
	ActiveIndicator = nullptr;
}

void UDFDamageDirectionWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	UImage* const Img = ActiveIndicator.Get();
	if (!Img)
	{
		return;
	}
	PulseElapsed += InDeltaTime;
	if (PulseElapsed >= PulseTotalDuration)
	{
		ClearPulse();
		return;
	}
	float Opacity = 0.f;
	if (PulseElapsed <= PulseFadeInDuration)
	{
		Opacity = PulsePeakOpacity * (PulseElapsed / FMath::Max(PulseFadeInDuration, KINDA_SMALL_NUMBER));
	}
	else
	{
		const float T = (PulseElapsed - PulseFadeInDuration) / FMath::Max(PulseFadeOutDuration, KINDA_SMALL_NUMBER);
		Opacity = PulsePeakOpacity * (1.f - FMath::Clamp(T, 0.f, 1.f));
	}
	Img->SetRenderOpacity(Opacity);
}
