class SCR_BoltAnimationComponentClass : WeaponAnimationComponentClass {}

class SCR_BoltAnimationComponent : WeaponAnimationComponent {
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "The particle effect that you want to play when the bolt is cycled", params: "ptc")]
	protected ResourceName m_sCasingEjectionAnimation;

	[Attribute(desc: "Effect position")]
	protected ref PointInfo m_effectPosition;

	[Attribute("10000", desc: "The max time (in ms) that the reload should be allowed to continue before attempting to exit in case of a bug. Must exceed the longest possible reload time.")]
	protected int m_sMaxReloadTime;

	[Attribute("4", desc: "The max ammo of the magazine, not including the chambered round. Used only to reset ammo after reload get interrupted.")]
	protected int m_sMaxAmmo;

	[Attribute(params: "et")]
	ResourceName m_sShellPrefab;

	[Attribute("", desc: "Name of bullet mesh that you would like to show/hide")]
	protected string m_sBulletMesh;

	[Attribute("", desc: "Name of casing mesh that you would like to show/hide")]
	protected string m_sCasingMesh;

	protected IEntity m_Owner;
	protected AnimationEventID m_setAmmoCount = -1;
	protected AnimationEventID m_checkMag = -1;
	protected AnimationEventID m_ejectRound = -1;
	protected AnimationEventID m_spawnRound = -1;
	protected AnimationEventID m_despawnRound = -1;
	protected AnimationEventID m_adjustAmmoCount = -1;
	protected AnimationEventID m_emptyMagAfterReloadCheck = -1;
	protected AnimationEventID m_closeBolt = -1;
	protected AnimationEventID m_startReloadTimer = -1;
	protected AnimationEventID m_Weapon_TriggerPulled = -1;

	protected TAnimGraphVariable m_animStopReloading = -1;
	protected TAnimGraphVariable m_animCloseBoltFlag = -1;
	protected TAnimGraphVariable m_playerAnimStopReloading = -1;
	protected TAnimGraphVariable m_playerAnimCloseBoltFlag = -1;

	protected typename magWellType;
	protected int currentAmmoCount = 0;
	protected int m_CMD_Weapon_Reload;
	protected float m_startTime = -1;
	protected bool wasMagEmptyAfterReload = false;
	protected int meshIdx = -1;
	protected int magSlotIndex = -1;
	protected int loopCounter = 0;
	protected bool m_isWeaponChambered = true;
	protected bool IsRackCmdPending = false;

	protected IEntity mosinRound = null;
	IEntity foundMag = null;
	protected MagazineComponent magazine = null;
	protected CharacterAnimationComponent animation = null;
	protected SCR_CharacterControllerComponent controller = null;

	void SCR_BoltAnimationComponent(IEntityComponentSource src, IEntity ent, IEntity parent) {
		m_Owner = ent;
		m_animStopReloading = BindBoolVariable("MosinStopReloading");
		m_animCloseBoltFlag = BindBoolVariable("MosinCloseBoltFlag");
		m_CMD_Weapon_Reload = BindCommand("CMD_Weapon_Reload");

		m_setAmmoCount = GameAnimationUtils.RegisterAnimationEvent("SetAmmoCount");
		m_adjustAmmoCount = GameAnimationUtils.RegisterAnimationEvent("AdjustAmmoCount");
		m_checkMag = GameAnimationUtils.RegisterAnimationEvent("CheckMag");
		m_ejectRound = GameAnimationUtils.RegisterAnimationEvent("EjectRound");
		m_spawnRound = GameAnimationUtils.RegisterAnimationEvent("SpawnRound");
		m_despawnRound = GameAnimationUtils.RegisterAnimationEvent("DespawnRound");
		m_emptyMagAfterReloadCheck = GameAnimationUtils.RegisterAnimationEvent("EmptyMagAfterReloadCheck");
		m_closeBolt = GameAnimationUtils.RegisterAnimationEvent("CloseBolt");
		m_startReloadTimer = GameAnimationUtils.RegisterAnimationEvent("StartReloadTimer");
		m_Weapon_TriggerPulled = GameAnimationUtils.RegisterAnimationEvent("Weapon_TriggerPulled");
	}
	
	override event void OnAnimationEvent(AnimationEventID animEventType, AnimationEventID animUserString, int intParam, float timeFromStart, float timeToEnd) {
		super.OnAnimationEvent(animEventType, animUserString, intParam, timeFromStart, timeToEnd);
	
		if (!m_Owner) return;
		WeaponComponent weapon = WeaponComponent.Cast(m_Owner.FindComponent(WeaponComponent));
		if (!weapon) return;

		IEntity parentEntity = m_Owner.GetParent();
		if (!parentEntity) return;

		ChimeraCharacter character = ChimeraCharacter.Cast(parentEntity);
		if (!character) return;

		controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		if (!controller) return;

		IEntity controllerOwner = controller.GetOwner();
		if (!controllerOwner) return;

		animation = CharacterAnimationComponent.Cast(controllerOwner.FindComponent(CharacterAnimationComponent));
		if (!animation) return;

		SCR_InventoryStorageManagerComponent invMan = SCR_InventoryStorageManagerComponent.Cast(controller.GetInventoryStorageManager());
		if (!invMan) return;

		if (!magazine) magazine = MagazineComponent.Cast(weapon.GetCurrentMagazine());

		if (magazine && magSlotIndex == -1) {
			IEntity magOwner = magazine.GetOwner();
			if (magOwner) {
				InventoryMagazineComponent magInvComp = InventoryMagazineComponent.Cast(magOwner.FindComponent(InventoryMagazineComponent));
				if (magInvComp) {
					InventoryStorageSlot parentSlot = magInvComp.GetParentSlot();
					if (parentSlot) {
						magSlotIndex = parentSlot.GetID();
					}
				}
			}
		}

		if (meshIdx == -1) {
			IEntity weaponOwner = weapon.GetOwner();
			if (weaponOwner)
				meshIdx = GameAnimationUtils.FindMeshIndex(weaponOwner, "Cartridge");
			else
				return;
		}

		m_playerAnimStopReloading = animation.BindVariableBool("MosinStopReloading");
		m_playerAnimCloseBoltFlag = animation.BindVariableBool("MosinCloseBoltFlag");

		BaseMuzzleComponent currMuzzle = weapon.GetCurrentMuzzle();
		if (!currMuzzle) return;
		m_isWeaponChambered = currMuzzle.IsCurrentBarrelChambered();

		if (!magWellType && magazine && magazine.GetMagazineWell()) {
			magWellType = magazine.GetMagazineWell().Type();
		}

		if (animEventType == m_Weapon_TriggerPulled) {
			TryRackBolt();
		}
		
		
		if (m_isWeaponChambered && IsRackCmdPending)
			IsRackCmdPending = false;

		if (animEventType == m_setAmmoCount) {
			currentAmmoCount += 1;
			IEntity weaponOwner = weapon.GetOwner();
			if (weaponOwner)
				GameAnimationUtils.ShowMesh(weaponOwner, meshIdx, true);

			if (magazine) {
				SetAmmoCount(magazine, currentAmmoCount);
				if (currentAmmoCount >= m_sMaxAmmo) {
					animation.SetVariableInt(m_playerAnimStopReloading, true);
					SetIntVariable(m_animStopReloading, true);
				}
			}
		}

		if (animEventType == m_checkMag) {
			SCR_MagazinePredicate predicate = new SCR_MagazinePredicate();
			predicate.magWellType = magWellType;
			IEntity newMag = invMan.FindItem(predicate);
			if (!newMag) {
				animation.SetVariableInt(m_playerAnimStopReloading, true);
				SetBoolVariable(m_animStopReloading, true);
			}
			if (magazine) {
				currentAmmoCount = magazine.GetAmmoCount();
				if (currentAmmoCount >= m_sMaxAmmo || !newMag) {
					animation.SetVariableInt(m_playerAnimStopReloading, true);
					SetIntVariable(m_animStopReloading, true);
				}
			}
		}

		if (animEventType == m_spawnRound) {
			Resource prefab = Resource.Load(m_sShellPrefab);
			if (!prefab) return;
			mosinRound = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld());
			if (!mosinRound || !controller) return;
			EntitySlotInfo leftHandSlot = controller.GetLeftHandPointInfo();
			if (leftHandSlot) leftHandSlot.AttachEntity(mosinRound);
			animation.SetVariableBool(m_playerAnimCloseBoltFlag, false);
			SetBoolVariable(m_animCloseBoltFlag, false);
		}

		if (animEventType == m_despawnRound) {
			if (loopCounter > m_sMaxAmmo) {
				animation.SetVariableInt(m_playerAnimStopReloading, true);
				SetBoolVariable(m_animStopReloading, true);
			}
			loopCounter += 1;

			if (mosinRound && controller) {
				EntitySlotInfo leftHandSlot = controller.GetLeftHandPointInfo();
				if (leftHandSlot) {
					leftHandSlot.AttachEntity(mosinRound);
					leftHandSlot.DetachEntity();
				}
				RplComponent.DeleteRplEntity(mosinRound, false);
				mosinRound = null;
			} else {
				animation.SetVariableInt(m_playerAnimStopReloading, true);
				SetIntVariable(m_animStopReloading, true);
			}

			SCR_MagazinePredicate predicate = new SCR_MagazinePredicate();
			predicate.magWellType = magWellType;
			IEntity newMag = invMan.FindItem(predicate);
			if (newMag) {
				RplComponent.DeleteRplEntity(newMag, false);
			} else {
				animation.SetVariableInt(m_playerAnimStopReloading, true);
				SetIntVariable(m_animStopReloading, true);
			}
		}

		if (animEventType == m_adjustAmmoCount) {
			if (magazine) {
				IEntity magOwner = magazine.GetOwner();
				if (magOwner) {
					if (m_sCasingMesh)
						GameAnimationUtils.ShowMesh(magOwner, GameAnimationUtils.FindMeshIndex(magOwner, m_sCasingMesh), false);
					if (m_sBulletMesh)
						GameAnimationUtils.ShowMesh(magOwner, GameAnimationUtils.FindMeshIndex(magOwner, m_sBulletMesh), false);
				}
			}
			if (wasMagEmptyAfterReload) {
				SetAmmoCount(magazine, 0);
				wasMagEmptyAfterReload = false;
			}
			if (magazine && magazine.GetAmmoCount() == 0) {
				IEntity weaponOwner = weapon.GetOwner();
				if (weaponOwner)
					GameAnimationUtils.ShowMesh(weaponOwner, meshIdx, false);
			}
		}

		if (animEventType == m_emptyMagAfterReloadCheck) {
			m_startTime = -1;
			animation.SetVariableInt(m_playerAnimStopReloading, true);
			SetIntVariable(m_animStopReloading, true);

			SCR_MagazinePredicate predicate = new SCR_MagazinePredicate();
			predicate.magWellType = magWellType;
			foundMag = invMan.FindItem(predicate);
			if (magazine && magazine.GetAmmoCount() == 0 && !foundMag) {
				SetAmmoCount(magazine, 1);
				wasMagEmptyAfterReload = true;
			}
		}

		if (animEventType == m_ejectRound && m_sCasingEjectionAnimation && m_sCasingEjectionAnimation.Length() > 0 && m_effectPosition && magazine) {
			ParticleEffectEntitySpawnParams spawnParams();
			spawnParams.Parent = magazine.GetOwner();
			vector mat[4];
			spawnParams.PivotID = m_effectPosition.GetNodeId();
			m_effectPosition.GetTransform(mat);
			spawnParams.Transform[0] = mat[0];
			spawnParams.Transform[1] = mat[1];
			spawnParams.Transform[2] = mat[2];
			spawnParams.Transform[3] = mat[3];
			ParticleEffectEntity.SpawnParticleEffect(m_sCasingEjectionAnimation, spawnParams);
		}

		if (animEventType == m_closeBolt) {
			animation.SetVariableBool(m_playerAnimCloseBoltFlag, true);
			SetBoolVariable(m_animCloseBoltFlag, true);
		}

		if (animEventType == m_startReloadTimer) {
			loopCounter = 0;
			m_startTime = GetGame().GetWorld().GetWorldTime();
			animation.SetVariableInt(m_playerAnimStopReloading, false);
			SetIntVariable(m_animStopReloading, false);
			animation.SetVariableBool(m_playerAnimCloseBoltFlag, false);
			SetBoolVariable(m_animCloseBoltFlag, false);
		}
	}
	
	private void TryRackBolt(){
		// To ensure that weapon is racked on the second trigger pull
		if (!m_isWeaponChambered && !IsRackCmdPending) {
			IsRackCmdPending = true;
			return;
		}
		IEntity parentEntity = m_Owner.GetParent();
		if (!parentEntity) 
			return;

		ChimeraCharacter character = ChimeraCharacter.Cast(parentEntity);
		if (!character) 
			return;

		SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());
		if (!controller) 
			return;
	
		WeaponComponent weapon = WeaponComponent.Cast(m_Owner.FindComponent(WeaponComponent));
		if (!weapon) 
			return;
		BaseMuzzleComponent muzzle = weapon.GetCurrentMuzzle();
		if (!muzzle || !magazine) return;
		
		IEntity controllerOwner = controller.GetOwner();
		if (!controllerOwner) return;

		CharacterAnimationComponent animation = CharacterAnimationComponent.Cast(controllerOwner.FindComponent(CharacterAnimationComponent));
		if (!animation) return;

		// TODO: This causes character reload command to sometimes only appear on the client side.
		if (!muzzle.IsCurrentBarrelChambered() && magazine.GetAmmoCount() > 0) {
			int reloadCmd = animation.BindCommand("CMD_BC_Weapon_Rack_Bolt");
			int weaponReloadCmd = BindCommand("CMD_BC_Weapon_Rack_Bolt");
			CallCommand(weaponReloadCmd, 1, 0.0);
			animation.CallCommand(reloadCmd, 1, 0.0);
			controller.ReloadWeapon();
		}
		IsRackCmdPending = false;
		return;
	}

	private void SetAmmoCount(MagazineComponent magazine, int ammoCount) {
		if (!magazine) return;
		IEntity magEntity = magazine.GetOwner();
		if (!magEntity) return;
		RplComponent rplComponent = RplComponent.Cast(magEntity.FindComponent(RplComponent));
		if (rplComponent && rplComponent.Role() == RplRole.Authority)
			magazine.SetAmmoCount(ammoCount);
	}
}
