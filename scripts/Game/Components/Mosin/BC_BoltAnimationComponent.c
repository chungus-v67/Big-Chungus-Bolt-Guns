class BC_BoltAnimationComponentClass : WeaponAnimationComponentClass {}

class BC_BoltAnimationComponent : WeaponAnimationComponent {
	[Attribute("", UIWidgets.ResourceNamePicker, desc:"The particle effect to play when the bolt is cycled", params:"ptc")]
	protected ResourceName m_sCasingEjectionAnimation;

	[Attribute(desc:"Effect position")]
	protected ref PointInfo m_effectPosition;

	[Attribute(params:"et")]
	ResourceName m_sShellPrefab;

	[Attribute("", desc:"Name of bullet mesh to show/hide")]
	protected string m_sBulletMesh;

	[Attribute("", desc:"Name of casing mesh to show/hide")]
	protected string m_sCasingMesh;

	protected IEntity m_Owner;
	protected IEntity m_ActiveRound = null;

	protected AnimationEventID m_setAmmoCount = -1;
	protected AnimationEventID m_checkMag = -1;
	protected AnimationEventID m_ejectRound = -1;
	protected AnimationEventID m_spawnRound = -1;
	protected AnimationEventID m_adjustAmmoCount = -1;
	protected AnimationEventID m_emptyMagAfterReloadCheck = -1;
	protected AnimationEventID m_closeAction = -1;
	protected AnimationEventID m_startReloadTimer = -1;
	protected AnimationEventID m_Weapon_Rack_Bolt = -1;
	protected AnimationEventID m_BC_PumpActionForward = -1;

	protected TAnimGraphVariable m_animStopReloading = -1;
	protected TAnimGraphVariable m_animCloseBoltFlag = -1;
	protected TAnimGraphVariable m_playerAnimStopReloading = -1;
	protected TAnimGraphVariable m_playerAnimCloseBoltFlag = -1;

	protected int m_sMaxAmmo = -1;
	protected int magSlotIndex = -1;
	protected typename magWellType;
	protected bool wasMagEmptyAfterReload = false;
	protected bool isWeaponStillReloading = false;
	protected int maxReloadDuration = 10000;
	protected int m_iCMD_Weapon_Reload = -1;
	protected bool m_bIsPumpingAction = false;

	void BC_BoltAnimationComponent(IEntityComponentSource src, IEntity ent, IEntity parent) {
		m_Owner = ent;

		m_iCMD_Weapon_Reload = BindCommand("CMD_Weapon_Reload");
		m_animStopReloading = BindBoolVariable("BoltActionStopReloading");
		m_animCloseBoltFlag = BindBoolVariable("BoltActionCloseBoltFlag");

		m_setAmmoCount = GameAnimationUtils.RegisterAnimationEvent("BoltActionSetAmmoCount");
		m_adjustAmmoCount = GameAnimationUtils.RegisterAnimationEvent("BoltActionAdjustAmmoCount");
		m_checkMag = GameAnimationUtils.RegisterAnimationEvent("BoltActionCheckMag");
		m_ejectRound = GameAnimationUtils.RegisterAnimationEvent("BoltActionEjectRound");
		m_spawnRound = GameAnimationUtils.RegisterAnimationEvent("BoltActionSpawnRound");
		m_emptyMagAfterReloadCheck = GameAnimationUtils.RegisterAnimationEvent("BoltActionEmptyMagAfterReloadCheck");
		m_closeAction = GameAnimationUtils.RegisterAnimationEvent("BoltActionTransition");
		m_Weapon_Rack_Bolt = GameAnimationUtils.RegisterAnimationEvent("Weapon_Rack_Bolt");
		m_startReloadTimer = GameAnimationUtils.RegisterAnimationEvent("StartReloadTimer");
		m_BC_PumpActionForward = GameAnimationUtils.RegisterAnimationEvent("BC_PumpActionForward");
	}

	override event void OnAnimationEvent(AnimationEventID animEventType, AnimationEventID animUserString, int intParam, float timeFromStart, float timeToEnd) {
		super.OnAnimationEvent(animEventType, animUserString, intParam, timeFromStart, timeToEnd);

		switch (animEventType) {
			case m_startReloadTimer:
				InitReloadSequence();
				break;
			case m_BC_PumpActionForward:
				m_bIsPumpingAction = false;
				break;
			case m_Weapon_Rack_Bolt:
				GetGame().GetCallqueue().CallLater(UpdateHud, 10, false, true);
				break;
			case m_setAmmoCount:
				InsertShell();
				UpdateHud();
				break;
			case m_adjustAmmoCount:
				FixAmmoCount();
				break;
			case m_ejectRound:
				EjectRound();
				break;
			case m_checkMag:
				UpdateReloadStatus();
				break;
			case m_spawnRound:
				GrabRound();
				break;
			case m_emptyMagAfterReloadCheck:
				CompleteReload();
				break;
			case m_closeAction:
				SetTransitionFlag(intParam);
				break;
		}
	}

	private void InitReloadSequence() {
		if (magSlotIndex == -1)
			magSlotIndex = Math.Max(GetMagSlotIdx(), 0);

		IEntity magEnt = GetMagEnt();
		if (magEnt) {
			if (m_sBulletMesh.Length())
				GameAnimationUtils.ShowMesh(magEnt, GameAnimationUtils.FindMeshIndex(magEnt, m_sBulletMesh), true);
			if (m_sCasingMesh.Length())
				GameAnimationUtils.ShowMesh(magEnt, GameAnimationUtils.FindMeshIndex(magEnt, m_sCasingMesh), true);
		}

		InitPlayerAnimVariables();
		InitMaxAmmo();
		SetTransitionFlag(0);
		SetAnimBoolVars(m_playerAnimStopReloading, m_animStopReloading, false);
		UpdateReloadStatus();
	}

	private void CompleteReload() {
		isWeaponStillReloading = false;
		SetAnimBoolVars(m_playerAnimStopReloading, m_animStopReloading, true);
		SetAnimBoolVars(m_playerAnimCloseBoltFlag, m_animCloseBoltFlag, true);

		MagazineComponent magComp = GetMagComp();
		if (!magComp)
			return;

		if (magComp.GetAmmoCount() == 0) {
			SetAmmoCount(magComp, 1);
			wasMagEmptyAfterReload = true;
		}
	}

	private void GrabRound() {
		if (m_ActiveRound) {
			SCR_InventoryStorageManagerComponent invMan = GetPlayerInvMan();
			if (invMan) {
				RplComponent rplA = RplComponent.Cast(m_ActiveRound.FindComponent(RplComponent));
				if (rplA)
					rplA.DeleteRplEntity(m_ActiveRound, false);
				m_ActiveRound = null;
			}
		}

		SCR_CharacterControllerComponent charController = GetCharController();
		if (!charController)
			return;

		EntitySlotInfo leftHandSlot = charController.GetLeftHandPointInfo();
		if (!leftHandSlot)
			return;

		Resource prefab = Resource.Load(m_sShellPrefab);
		if (!prefab)
			return;

		EntitySpawnParams spawnParams = new EntitySpawnParams();
		m_ActiveRound = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld(), spawnParams);
		if (!m_ActiveRound)
			return;

		leftHandSlot.AttachEntity(m_ActiveRound);
		RplComponent rplComponent = RplComponent.Cast(m_ActiveRound.FindComponent(RplComponent));
		if (rplComponent)
			rplComponent.InsertToReplication();

		UpdateReloadStatus(true);
	}

	private void InsertShell() {
		DespawnRound();

		int currentAmmoCount = GetCurrentAmmoCount() + 1;
		MagazineComponent magComp = GetMagComp();
		if (!magComp)
			return;

		SetAmmoCount(magComp, currentAmmoCount);
		UpdateReloadStatus();
	}
	
	// Delete round
	private void DespawnRound() {
        SCR_InventoryStorageManagerComponent invMan = GetPlayerInvMan();
		if (!invMan)
			return;
		if (m_sBulletMesh.Length()){
			int bulletIdx = GameAnimationUtils.FindMeshIndex(m_ActiveRound, m_sBulletMesh);
			GameAnimationUtils.ShowMesh(m_ActiveRound, bulletIdx , false);
		}
		if (m_sCasingMesh.Length()){
			int casingIdx = GameAnimationUtils.FindMeshIndex(m_ActiveRound, m_sCasingMesh);
			GameAnimationUtils.ShowMesh(m_ActiveRound, casingIdx , false);
		}
        invMan.AskServerToDeleteEntity(m_ActiveRound);
       	RplComponent rplComponent = RplComponent.Cast(m_ActiveRound.FindComponent(BaseRplComponent));
       	if(rplComponent)
        	rplComponent.DeleteRplEntity(m_ActiveRound, false);
        m_ActiveRound = null; 
    }
	

	void UpdateReloadStatus(bool isCalledEarly = false) {
		int currentAmmoCount = GetCurrentAmmoCount();
		if (isCalledEarly)
			currentAmmoCount += 1;

		if (currentAmmoCount >= m_sMaxAmmo)
			SetAnimBoolVars(m_playerAnimStopReloading, m_animStopReloading, true);

		IEntity magEnt = GetMagFromInv();
		if (!magEnt)
			SetAnimBoolVars(m_playerAnimStopReloading, m_animStopReloading, true);
	}

	bool TryRackBolt() {
		if (IsWeaponChambered() || m_bIsPumpingAction || IsWeaponEmpty())
			return false;

		SCR_CharacterControllerComponent controller = GetCharController();
		if (!controller)
			return false;

		CharacterAnimationComponent animation = GetCharAnimComp();
		if (!animation)
			return false;

		if (!IsWeaponChambered() && GetCurrentAmmoCount() > 0) {
			int reloadCmd = animation.BindCommand("CMD_BC_Weapon_Rack_Bolt");
			int weaponReloadCmd = BindCommand("CMD_BC_Weapon_Rack_Bolt");
			m_bIsPumpingAction = true;
			CallCommand(weaponReloadCmd, 1, 0.0);
			animation.CallCommand(reloadCmd, 1, 0.0);
			return true;
		}
		return false;
	}

	private void FixAmmoCount() {
		SetAnimBoolVars(m_playerAnimStopReloading, m_animStopReloading, true);
		SetAnimBoolVars(m_playerAnimCloseBoltFlag, m_animCloseBoltFlag, true);

		MagazineComponent magComp = GetMagComp();
		if (!magComp)
			return;

		if (wasMagEmptyAfterReload) {
			SetAmmoCount(magComp, 0);
			wasMagEmptyAfterReload = false;
			UpdateHud();
		}
	}

	private void EjectRound() {
		IEntity magEnt = GetMagEnt();
		if (!magEnt)
			return;
		if (!m_sCasingEjectionAnimation.Length())
			return;

		ParticleEffectEntitySpawnParams spawnParams();
		spawnParams.Parent = magEnt;
		vector mat[4];
		spawnParams.PivotID = m_effectPosition.GetNodeId();
		m_effectPosition.GetTransform(mat);
		spawnParams.Transform[0] = mat[0];
		spawnParams.Transform[1] = mat[1];
		spawnParams.Transform[2] = mat[2];
		spawnParams.Transform[3] = mat[3];
		ParticleEffectEntity.SpawnParticleEffect(m_sCasingEjectionAnimation, spawnParams);
	}

	private void SetTransitionFlag(int intParam) {
		if (intParam == 1)
			SetAnimBoolVars(m_playerAnimCloseBoltFlag, m_animCloseBoltFlag, true);
		else
			SetAnimBoolVars(m_playerAnimCloseBoltFlag, m_animCloseBoltFlag, false);
	}

	private void UpdateHud(bool shouldUpdateAmmoCount = true) {
		if (!IsLocalPlayer())
			return;

		SCR_WeaponInfo weaponInfoHud = getWeaponInfoHud();
		if (!weaponInfoHud)
			return;

		int currentAmmoCount = GetCurrentAmmoCount();

		if (IsWeaponChambered()) {
			weaponInfoHud.m_Widgets.m_wFiremodeIcon.SetOpacity(1.0);
			weaponInfoHud.m_Widgets.m_wMagazineOutline.SetOpacity(1.0);
		} else {
			weaponInfoHud.m_Widgets.m_wFiremodeIcon.SetOpacity(weaponInfoHud.FADED_OPACITY);
			currentAmmoCount -= 1;
		}

		if (shouldUpdateAmmoCount) {
			if (m_sMaxAmmo == -1)
				InitMaxAmmo();

			if (currentAmmoCount < 0) {
				weaponInfoHud.m_Widgets.m_wMagazineProgress.SetMaskProgress(0.0);
			} else if (currentAmmoCount == 0) {
				weaponInfoHud.m_Widgets.m_wMagazineProgress.SetMaskProgress(0.0);
				weaponInfoHud.m_Widgets.m_wFiremodeIcon.SetOpacity(1.0);
			} else {
				weaponInfoHud.m_Widgets.m_wMagazineProgress.SetMaskProgress(currentAmmoCount / m_sMaxAmmo);
			}
		}
	}

	private void InitPlayerAnimVariables() {
		CharacterAnimationComponent charAnimComp = GetCharAnimComp();
		if (m_playerAnimStopReloading == -1)
			m_playerAnimStopReloading = charAnimComp.BindVariableBool("BoltActionStopReloading");
		if (m_playerAnimCloseBoltFlag == -1)
			m_playerAnimCloseBoltFlag = charAnimComp.BindVariableBool("BoltActionCloseBoltFlag");
	}

	private bool TryAttachMagToWeapon(IEntity magEntity) {
		if (!magEntity)
			return false;

		WeaponComponent weapon = GetWeaponComp();
		if (!weapon)
			return false;

		SCR_WeaponAttachmentsStorageComponent attachmentStorage = SCR_WeaponAttachmentsStorageComponent.Cast(weapon.GetOwner().FindComponent(SCR_WeaponAttachmentsStorageComponent));
		if (!attachmentStorage)
			return false;

		InventoryStorageSlot magSlot = attachmentStorage.GetSlot(magSlotIndex);
		if (!magSlot)
			return false;

		magSlot.AttachEntity(magEntity);
		return true;
	}

	private void EmergencyExitReload() {
		if (isWeaponStillReloading)
			SetAnimBoolVars(m_playerAnimStopReloading, m_animStopReloading, true);
	}

	bool GetIsWeaponReloadingStatus() { return isWeaponStillReloading; }
	bool GetMaxReloadDuration() { return maxReloadDuration; }

	private SCR_WeaponInfo getWeaponInfoHud() {
		SCR_HUDManagerComponent hudManager = SCR_HUDManagerComponent.GetHUDManager();
		if (!hudManager)
			return null;
		return SCR_WeaponInfo.Cast(hudManager.FindInfoDisplay(SCR_WeaponInfo));
	}

	private IEntity GetMagFromInv() {
		SCR_InventoryStorageManagerComponent invMan = GetPlayerInvMan();
		if (!invMan)
			return null;

		if (!magWellType && GetMagComp())
			magWellType = GetMagwellType();

		SCR_MagazinePredicate predicate = new SCR_MagazinePredicate();
		predicate.magWellType = magWellType;

		IEntity magEntity = invMan.FindItem(predicate);
		return magEntity;
	}

	private bool IsLocalPlayer() {
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return false;
		IEntity playerControllerEnt = playerController.GetControlledEntity();
		if (!playerControllerEnt)
			return false;
		IEntity weapon = GetOwner();
		if (!weapon)
			return false;
		IEntity characterEnt = weapon.GetParent();
		if (!characterEnt)
			return false;
		return playerControllerEnt == characterEnt;
	}

	private int GetCurrentAmmoCount() {
		MagazineComponent magComp = GetMagComp();
		if (!magComp)
			return 0;
		return magComp.GetAmmoCount();
	}

	private void InitMaxAmmo() {
		if (m_sMaxAmmo != -1)
			return;
		MagazineComponent mag = GetMagComp();
		if (!mag)
			return;
		m_sMaxAmmo = mag.GetMaxAmmoCount();
	}

	private WeaponComponent GetWeaponComp() {
		if (!m_Owner)
			return null;
		return WeaponComponent.Cast(m_Owner.FindComponent(WeaponComponent));
	}

	private ChimeraCharacter GetChimeraChar() {
		if (!m_Owner)
			return null;
		return ChimeraCharacter.Cast(m_Owner.GetParent());
	}

	private SCR_CharacterControllerComponent GetCharController() {
		ChimeraCharacter chimeraChar = GetChimeraChar();
		if (!chimeraChar)
			return null;
		return SCR_CharacterControllerComponent.Cast(chimeraChar.GetCharacterController());
	}

	private IEntity GetCharEnt() {
		if (!m_Owner)
			return null;
		return m_Owner.GetParent();
	}

	private CharacterAnimationComponent GetCharAnimComp() {
		IEntity playerEnt = GetCharEnt();
		if (!playerEnt)
			return null;
		return CharacterAnimationComponent.Cast(playerEnt.FindComponent(CharacterAnimationComponent));
	}

	private SCR_InventoryStorageManagerComponent GetPlayerInvMan() {
		SCR_CharacterControllerComponent charController = GetCharController();
		if (!charController)
			return null;
		return SCR_InventoryStorageManagerComponent.Cast(charController.GetInventoryStorageManager());
	}

	private MagazineComponent GetMagComp() {
		WeaponComponent weaponComp = GetWeaponComp();
		if (!weaponComp)
			return null;
		return MagazineComponent.Cast(weaponComp.GetCurrentMagazine());
	}

	private IEntity GetMagEnt() {
		MagazineComponent magComp = GetMagComp();
		if (!magComp)
			return null;
		return magComp.GetOwner();
	}

	private typename GetMagwellType() {
		MagazineComponent magComp = GetMagComp();
		if (!magComp)
			return typename.Empty;
		BaseMagazineWell magWell = magComp.GetMagazineWell();
		if (!magWell)
			return typename.Empty;
		return magWell.Type();
	}

	int GetMagSlotIdx() {
		IEntity magEnt = GetMagEnt();
		if (!magEnt)
			return -1;
		InventoryMagazineComponent magInvComp = InventoryMagazineComponent.Cast(magEnt.FindComponent(InventoryMagazineComponent));
		if (!magInvComp)
			return -1;
		InventoryStorageSlot magParentSlot = magInvComp.GetParentSlot();
		if (!magParentSlot)
			return -1;
		return magParentSlot.GetID();
	}

	private MuzzleComponent GetCurrentMuzzle() {
		WeaponComponent weaponComp = GetWeaponComp();
		if (!weaponComp)
			return null;
		return MuzzleComponent.Cast(weaponComp.GetCurrentMuzzle());
	}

	private bool IsWeaponChambered() {
		MuzzleComponent muzzleComp = GetCurrentMuzzle();
		if (!muzzleComp)
			return false;
		return muzzleComp.IsCurrentBarrelChambered();
	}

	private bool IsWeaponEmpty() {
		MagazineComponent magComp = GetMagComp();
		if (!magComp)
			return true;
		return magComp.GetAmmoCount() == 0;
	}

	private void SetAnimBoolVars(TAnimGraphVariable playerVar, TAnimGraphVariable weaponVar, bool value) {
		if (playerVar != -1) {
			CharacterAnimationComponent charAnimComp = GetCharAnimComp();
			if (charAnimComp)
				charAnimComp.SetVariableBool(playerVar, value);
		}
		SetBoolVariable(weaponVar, value);
	}

	private bool HasAttachedMagazine() {
		if (!m_Owner)
			return false;
		IEntity child = m_Owner.GetChildren();
		while (child) {
			if (child.FindComponent(MagazineComponent))
				return true;
			child = child.GetSibling();
		}
		return false;
	}

	private void SetAmmoCount(MagazineComponent magazine, int ammoCount) {
		if (!magazine)
			return;
		IEntity magEntity = magazine.GetOwner();
		if (!magEntity)
			return;
		RplComponent rplComponent = RplComponent.Cast(magEntity.FindComponent(RplComponent));
		if (rplComponent && rplComponent.Role() == RplRole.Authority)
			magazine.SetAmmoCount(ammoCount);
	}
}