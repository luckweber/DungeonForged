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

	UImage* Target = Indicator_Radial.Get();
	if (Target)
	{
		const float AngleDeg = FMath::RadiansToDegrees(FMath::Atan2(Side, Dot));
		Target->SetRenderTransformAngle(AngleDeg - 90.f);
	}
	else
	{
		Target = ResolveCardinalIndicator(Dot, Side);
	}
	StartPulseOnImage(Target, Intensity);
}

UImage* UDFDamageDirectionWidget::ResolveCardinalIndicator(const float Dot, const float Side) const
{
	if (FMath::Abs(Dot) > FMath::Abs(Side))
	{
		return Dot > 0.f ? Indicator_Top.Get() : Indicator_Bottom.Get();
	}
	return Side > 0.f ? Indicator_Right.Get() : Indicator_Left.Get();
}

void UDFDamageDirectionWidget::StartPulseOnImage(UImage* const Target, const float Intensity)
{
	if (!Target)
	{
		return;
	}

	for (FDamagePulseSlot& PulseSlot : ActivePulses)
	{
		if (PulseSlot.Image.Get() == Target)
		{
			PulseSlot.Elapsed = 0.f;
			PulseSlot.TotalDuration = PulseFadeInDuration + PulseFadeOutDuration;
			PulseSlot.PeakOpacity = FMath::Clamp(Intensity, 0.15f, 1.f);
			Target->SetRenderOpacity(PulseSlot.PeakOpacity);
			Target->SetVisibility(ESlateVisibility::HitTestInvisible);
			return;
		}
	}

	for (FDamagePulseSlot& PulseSlot : ActivePulses)
	{
		if (!PulseSlot.Image.IsValid())
		{
			PulseSlot.Image = Target;
			PulseSlot.Elapsed = 0.f;
			PulseSlot.TotalDuration = PulseFadeInDuration + PulseFadeOutDuration;
			PulseSlot.PeakOpacity = FMath::Clamp(Intensity, 0.15f, 1.f);
			Target->SetRenderOpacity(PulseSlot.PeakOpacity);
			Target->SetVisibility(ESlateVisibility::HitTestInvisible);
			return;
		}
	}

	if (ActivePulses.Num() >= MaxConcurrentPulses)
	{
		ActivePulses.RemoveAt(0);
	}
	FDamagePulseSlot NewSlot;
	NewSlot.Image = Target;
	NewSlot.Elapsed = 0.f;
	NewSlot.TotalDuration = PulseFadeInDuration + PulseFadeOutDuration;
	NewSlot.PeakOpacity = FMath::Clamp(Intensity, 0.15f, 1.f);
	Target->SetRenderOpacity(NewSlot.PeakOpacity);
	Target->SetVisibility(ESlateVisibility::HitTestInvisible);
	ActivePulses.Add(NewSlot);
}

void UDFDamageDirectionWidget::HideImage(UImage* const Image) const
{
	if (!Image)
	{
		return;
	}
	Image->SetRenderOpacity(0.f);
	Image->SetVisibility(ESlateVisibility::Collapsed);
}

void UDFDamageDirectionWidget::StepPulseSlot(FDamagePulseSlot& PulseSlot, const float DeltaTime)
{
	UImage* const Img = PulseSlot.Image.Get();
	if (!Img)
	{
		return;
	}
	PulseSlot.Elapsed += DeltaTime;
	if (PulseSlot.Elapsed >= PulseSlot.TotalDuration)
	{
		HideImage(Img);
		PulseSlot.Image = nullptr;
		return;
	}
	float Opacity = 0.f;
	if (PulseSlot.Elapsed <= PulseFadeInDuration)
	{
		Opacity = PulseSlot.PeakOpacity * (PulseSlot.Elapsed / FMath::Max(PulseFadeInDuration, KINDA_SMALL_NUMBER));
	}
	else
	{
		const float T = (PulseSlot.Elapsed - PulseFadeInDuration) / FMath::Max(PulseFadeOutDuration, KINDA_SMALL_NUMBER);
		Opacity = PulseSlot.PeakOpacity * (1.f - FMath::Clamp(T, 0.f, 1.f));
	}
	Img->SetRenderOpacity(Opacity);
}

void UDFDamageDirectionWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	for (FDamagePulseSlot& PulseSlot : ActivePulses)
	{
		StepPulseSlot(PulseSlot, InDeltaTime);
	}
	ActivePulses.RemoveAll([](const FDamagePulseSlot& PulseSlot) { return !PulseSlot.Image.IsValid(); });
}
