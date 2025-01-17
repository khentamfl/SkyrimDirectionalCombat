#include "AttackHandler.h"
#include "SettingsLoader.h"
#include "DirectionHandler.h"

constexpr float NPCLockoutTime = 0.15f;

bool AttackHandler::InChamberWindow(RE::Actor* actor)
{
	bool ret = false;
	ChamberWindowMtx.lock_shared();
	ret = ChamberWindow.contains(actor->GetHandle());
	ChamberWindowMtx.unlock_shared();
	return ret;
}

bool AttackHandler::InFeintWindow(RE::Actor* actor)
{
	bool ret = false;
	FeintWindowMtx.lock_shared();
	ret = FeintWindow.contains(actor->GetHandle());
	FeintWindowMtx.unlock_shared();
	return ret;
}

void AttackHandler::RemoveFeintWindow(RE::Actor* actor)
{
	FeintWindowMtx.lock_shared();
	FeintWindow.erase(actor->GetHandle());
	FeintWindowMtx.unlock_shared();
}

bool AttackHandler::CanAttack(RE::Actor* actor)
{
	bool ret = false;
	if (actor->IsPlayerRef())
	{
		PlayerMtx.lock_shared();
		ret = !PlayerLockout;
		PlayerMtx.unlock_shared();
	}
	else
	{
		AttackLockoutMtx.lock_shared();
		ret = !AttackLockout.contains(actor->GetHandle());
		AttackLockoutMtx.unlock_shared();
	}
	return ret;
}

void AttackHandler::HandleFeint(RE::Actor* actor)
{
	FeintWindowMtx.lock();
	if (FeintWindow.contains(actor->GetHandle()))
	{
		actor->NotifyAnimationGraph("attackStop");
		FeintWindow.erase(actor->GetHandle());
		actor->AsActorState()->actorState1.meleeAttackState = RE::ATTACK_STATE_ENUM::kNone;
		
		/*
		
		Directions dir = DirectionHandler::GetSingleton()->GetCurrentDirection(actor);
		if (dir == Directions::TR || dir == Directions::BR)
		{
			DirectionHandler::GetSingleton()->WantToSwitchTo(actor, Directions::TL, true);
		}
		else
		{
			DirectionHandler::GetSingleton()->WantToSwitchTo(actor, Directions::TR, true);
		}
		*/

		
	}
	FeintWindowMtx.unlock();
}

void AttackHandler::AddChamberWindow(RE::Actor* actor)
{
	// masterstrike window must be a fixed time, otherwise slower attack speeds actually make it easier to masterstrike
	ChamberWindowMtx.lock();
	ChamberWindow[actor->GetHandle()] = DifficultySettings::ChamberWindowTime;
	ChamberWindowMtx.unlock();
}

void AttackHandler::AddFeintWindow(RE::Actor* actor)
{
	FeintWindowMtx.lock();
	FeintWindow[actor->GetHandle()] = DifficultySettings::FeintWindowTime;
	FeintWindowMtx.unlock();
}

void AttackHandler::AddNPCSmallLockout(RE::Actor* actor)
{
	AttackLockoutMtx.lock();
	if (AttackLockout.contains(actor->GetHandle()))
	{
		AttackLockout[actor->GetHandle()] = NPCLockoutTime;
	}
	AttackLockoutMtx.unlock();
}

void AttackHandler::AddLockout(RE::Actor* actor)
{
	AttackLockoutMtx.lock();
	AttackLockout[actor->GetHandle()] = DifficultySettings::AttackTimeoutTime;
	AttackLockoutMtx.unlock();
}

void AttackHandler::LockoutPlayer()
{
	PlayerMtx.lock();
	PlayerLockout = true;
	PlayerLockoutTimer = DifficultySettings::AttackTimeoutTime;
	PlayerMtx.unlock();
}

void AttackHandler::Cleanup()
{
	PlayerMtx.lock();
	PlayerLockout = false;
	PlayerMtx.unlock();

	ChamberWindowMtx.lock();
	ChamberWindow.clear();
	ChamberWindowMtx.unlock();

	AttackLockoutMtx.lock();
	AttackLockout.clear();
	AttackLockoutMtx.unlock();

	FeintWindowMtx.lock();
	FeintWindow.clear();
	FeintWindowMtx.unlock();
}

void AttackHandler::RemoveActor(RE::ActorHandle actor)
{

	ChamberWindowMtx.lock();
	ChamberWindow.erase(actor);
	ChamberWindowMtx.unlock();
	AttackLockoutMtx.lock();
	AttackLockout.erase(actor);
	AttackLockoutMtx.unlock();
	FeintWindowMtx.lock();
	FeintWindow.erase(actor);
	FeintWindowMtx.unlock();
}

void AttackHandler::Update(float delta)
{
	PlayerMtx.lock();
	if (PlayerLockout)
	{
		PlayerLockoutTimer -= delta;
		if (PlayerLockoutTimer < 0.f)
		{
			PlayerLockout = false;
		}
	}
	PlayerMtx.unlock();

	ChamberWindowMtx.lock();
	auto ChamberIter = ChamberWindow.begin();
	while (ChamberIter != ChamberWindow.end())
	{
		if (!ChamberIter->first)
		{
			ChamberIter = ChamberWindow.erase(ChamberIter);
			continue;
		}
		RE::Actor* actor = ChamberIter->first.get().get();
		if (!actor)
		{
			ChamberIter = ChamberWindow.erase(ChamberIter);
			continue;
		}
		ChamberIter->second -= delta;
		if (ChamberIter->second <= 0)
		{
			ChamberIter = ChamberWindow.erase(ChamberIter);
			continue;
		}
		ChamberIter++;
	}
	ChamberWindowMtx.unlock();

	AttackLockoutMtx.lock();
	auto AttackIter = AttackLockout.begin();
	while (AttackIter != AttackLockout.end())
	{
		if (!AttackIter->first)
		{
			AttackIter = AttackLockout.erase(AttackIter);
			continue;
		}
		RE::Actor* actor = AttackIter->first.get().get();
		if (!actor)
		{
			AttackIter = AttackLockout.erase(AttackIter);
			continue;
		}
		AttackIter->second -= delta;
		if (AttackIter->second <= 0)
		{
			AttackIter = AttackLockout.erase(AttackIter);
			continue;
		}
		AttackIter++;
	}
	AttackLockoutMtx.unlock();

	FeintWindowMtx.lock();
	auto FeintIter = FeintWindow.begin();
	while (FeintIter != FeintWindow.end())
	{
		if (!FeintIter->first)
		{
			FeintIter = FeintWindow.erase(FeintIter);
			continue;
		}
		RE::Actor* actor = FeintIter->first.get().get();
		if (!actor)
		{
			FeintIter = FeintWindow.erase(FeintIter);
			continue;
		}
		FeintIter->second -= delta;
		if (FeintIter->second <= 0)
		{
			FeintIter = FeintWindow.erase(FeintIter);
			continue;
		}
		FeintIter++;
	}
	FeintWindowMtx.unlock();
}