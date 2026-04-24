// Source/DungeonForged/Private/FX/UDFCameraShakes.cpp
#include "FX/UDFCameraShakes.h"

namespace
{
	void InitFF(FFOscillator& O, const float Amp, const float Freq)
	{
		O.Amplitude = Amp;
		O.Frequency = Freq;
		O.InitialOffset = EInitialOscillatorOffset::EOO_OffsetRandom;
		O.Waveform = EOscillatorWaveform::SineWave;
	}
}

UDFCameraShake_LightHit::UDFCameraShake_LightHit()
{
	OscillationDuration = 0.2f;
	OscillationBlendInTime = 0.01f;
	OscillationBlendOutTime = 0.05f;
	InitFF(RotOscillation.Pitch, 0.8f, 30.f);
}

UDFCameraShake_HeavyHit::UDFCameraShake_HeavyHit()
{
	OscillationDuration = 0.35f;
	OscillationBlendInTime = 0.01f;
	OscillationBlendOutTime = 0.08f;
	InitFF(RotOscillation.Pitch, 2.f, 20.f);
	InitFF(RotOscillation.Roll, 2.f, 20.f);
	InitFF(LocOscillation.Y, 3.f, 25.f);
}

UDFCameraShake_BossSlam::UDFCameraShake_BossSlam()
{
	OscillationDuration = 0.6f;
	OscillationBlendInTime = 0.02f;
	OscillationBlendOutTime = 0.1f;
	InitFF(RotOscillation.Pitch, 4.f, 15.f);
	InitFF(RotOscillation.Yaw, 4.f, 15.f);
	InitFF(RotOscillation.Roll, 4.f, 15.f);
	InitFF(LocOscillation.Z, 8.f, 10.f);
}

UDFCameraShake_Explosion::UDFCameraShake_Explosion()
{
	OscillationDuration = 0.8f;
	OscillationBlendInTime = 0.01f;
	OscillationBlendOutTime = 0.12f;
	InitFF(RotOscillation.Pitch, 5.f, 12.f);
	InitFF(RotOscillation.Yaw, 5.f, 12.f);
	InitFF(RotOscillation.Roll, 5.f, 12.f);
	InitFF(LocOscillation.X, 5.f, 12.f);
	InitFF(LocOscillation.Y, 5.f, 12.f);
	InitFF(LocOscillation.Z, 5.f, 12.f);
}
