modded class SCR_CharacterControllerComponent {
    protected AnimationEventID m_spawnRound;
    protected AnimationEventID m_insertShell;
    protected AnimationEventID m_startReloadTimer;
	 protected AnimationEventID m_emptyMagAfterReloadCheck = -1;
	
	protected TAnimGraphVariable m_playerAnimStopReloading = -1;
    protected TAnimGraphVariable m_playerAnimCloseActionFlag = -1;
	
	protected bool isWeaponStillReloading = false; // Used for emergency exit
    protected IEntity shotgunRoundActiveEnt;
    bool flag = false;

    override protected void OnInit(IEntity owner) {
        super.OnInit(owner);
        m_spawnRound = GameAnimationUtils.RegisterAnimationEvent("BoltActionSpawnRound");
        m_insertShell = GameAnimationUtils.RegisterAnimationEvent("BoltActionSetAmmoCount");
        m_startReloadTimer = GameAnimationUtils.RegisterAnimationEvent("StartReloadTimer");
		m_emptyMagAfterReloadCheck = GameAnimationUtils.RegisterAnimationEvent("BoltActionEmptyMagAfterReloadCheck");
    }

    override protected void OnAnimationEvent(AnimationEventID animEventType,AnimationEventID animUserString,int intParam,float timeFromStart,float timeToEnd) {
        super.OnAnimationEvent(animEventType,animUserString,intParam,timeFromStart,timeToEnd);

        if(animEventType == m_spawnRound)
           	GrabRound_BoltAction();
		
        if(animEventType == m_insertShell)
           	InsertShell_BoltAction();
		
        if(animEventType == m_startReloadTimer)
           	InitReloadSequence_BoltAction();
		
		if(animEventType == m_emptyMagAfterReloadCheck)
           	CompleteReload_BoltAction();
    }
	
	private CharacterAnimationComponent GetCharAnimComp_BoltAction() {
        IEntity playerEnt = GetOwner();
        if(!playerEnt)
            return null;
        return CharacterAnimationComponent.Cast(playerEnt.FindComponent(CharacterAnimationComponent));
    }
	
	private void InitPlayerAnimVariables_BoltAction() {
        CharacterAnimationComponent charAnimComp = GetCharAnimComp_BoltAction();
		if (!charAnimComp)
			return;
        if(m_playerAnimStopReloading == -1)
            m_playerAnimStopReloading = charAnimComp.BindVariableBool("BoltActionStopReloading");
        if(m_playerAnimCloseActionFlag == -1)
            m_playerAnimCloseActionFlag = charAnimComp.BindVariableBool("BoltActionCloseBoltFlag");
    }


    private void InitReloadSequence_BoltAction() {
        MagazineComponent magComp = GetMagComp_BoltAction();
        if(!magComp) {
            IEntity newMag = FindMatchingMagInInventory_BoltAction();
            TryAttachMagToWeapon_BoltAction(newMag);
        }
		CharacterAnimationComponent charAnimComp = GetCharAnimComp_BoltAction();
		isWeaponStillReloading = true;
		InitPlayerAnimVariables_BoltAction();
		// QueueEmergencyExit_BoltAction(); Can get errantly queued on a false reload. Needs another route.
    }
	
	private void CompleteReload_BoltAction() {
		isWeaponStillReloading = false;
    }
	
	protected void QueueEmergencyExit_BoltAction(){
		BC_BoltAnimationComponent pumpShotgunComp = GetPumpShotgunComp_BoltAction();
		if (!pumpShotgunComp)
			return;
		GetGame().GetCallqueue().CallLater(EmergencyExitReload_BoltAction, pumpShotgunComp.GetMaxReloadDuration());
	}
	
	private void EmergencyExitReload_BoltAction(){
		BC_BoltAnimationComponent pumpShotgunComp = GetPumpShotgunComp_BoltAction();
		if (!pumpShotgunComp)
			return;
		
		CharacterAnimationComponent charAnimComp = GetCharAnimComp_BoltAction();
		if (!charAnimComp)
			return;
		
		if (isWeaponStillReloading){
			charAnimComp.SetVariableBool(m_playerAnimStopReloading, true); 
			charAnimComp.SetVariableBool(m_playerAnimCloseActionFlag, true);
		}
	}

    protected void GrabRound_BoltAction() {
        if(shotgunRoundActiveEnt)
            InsertShell_BoltAction();

        SCR_InventoryStorageManagerComponent invMan = GetInvMan_BoltAction();
        if(!invMan)
            return;

        IEntity foundMag = FindMatchingMagInInventory_BoltAction();
        if(!foundMag)
            return;

        shotgunRoundActiveEnt = foundMag;
    }
	
	protected BC_BoltAnimationComponent GetPumpShotgunComp_BoltAction(){
		IEntity weaponEnt = GetWeaponEnt_BoltAction();
		if (!weaponEnt)
			return null;
		return BC_BoltAnimationComponent.Cast(weaponEnt.FindComponent(BC_BoltAnimationComponent));
	}

    protected void InsertShell_BoltAction() {
        if(!shotgunRoundActiveEnt)
            return;

        SCR_InventoryStorageManagerComponent invMan = GetInvMan_BoltAction();
        if(!invMan)
            return;

		MagazineComponent magComp = GetMagComp_BoltAction();

		GameAnimationUtils.ShowMesh(shotgunRoundActiveEnt, GameAnimationUtils.FindMeshIndex(shotgunRoundActiveEnt,"Shell"),false);
		if (shotgunRoundActiveEnt){
			RplComponent.DeleteRplEntity(shotgunRoundActiveEnt, false);
		}
        shotgunRoundActiveEnt = null;
		
    }

    bool TryAttachMagToWeapon_BoltAction(IEntity magEntity) {
        if(!magEntity)
            return false;

        WeaponComponent weapon = GetWeaponComp_BoltAction();
        if(!weapon)
            return false;

        SCR_WeaponAttachmentsStorageComponent attachmentStorage = SCR_WeaponAttachmentsStorageComponent.Cast(
            weapon.GetOwner().FindComponent(SCR_WeaponAttachmentsStorageComponent)
        );
        if(!attachmentStorage)
            return false;

        InventoryStorageSlot magSlot = attachmentStorage.GetSlot(0);
        if(!magSlot)
            return false;

        magSlot.AttachEntity(magEntity);
        int res = ReloadWeaponWith(magEntity, true);
        return res;
    }

    private WeaponComponent GetWeaponComp_BoltAction() {
        BaseWeaponManagerComponent weaponManager = GetWeaponManagerComponent();
        if(!weaponManager)
            return null;
        return WeaponComponent.Cast(weaponManager.GetCurrentWeapon());
    }
	
	private IEntity GetWeaponEnt_BoltAction() {
        WeaponComponent weaponComp = GetWeaponComp_BoltAction();
        if(!weaponComp)
            return null;
        return weaponComp.GetOwner();
    }
	
    private MagazineComponent GetMagComp_BoltAction() {
        WeaponComponent weapon = GetWeaponComp_BoltAction();
        if(!weapon)
            return null;
        return MagazineComponent.Cast(weapon.GetCurrentMagazine());
    }

    private typename GetMagWellType_BoltAction() {
		IEntity weaponEnt = GetWeaponEnt_BoltAction();
		if (!weaponEnt)
			return typename.Empty;
		MuzzleComponent muzzleComp = MuzzleComponent.Cast(weaponEnt.FindComponent(MuzzleComponent));
		if (!muzzleComp)
			return typename.Empty;
		
		BaseMagazineWell weaponMagWell = muzzleComp.GetMagazineWell();
		if (!weaponMagWell)
			return typename.Empty;
		
		return weaponMagWell.Type();
    }

    private SCR_InventoryStorageManagerComponent GetInvMan_BoltAction() {
        return SCR_InventoryStorageManagerComponent.Cast(GetInventoryStorageManager());
    }

    private SCR_MagazinePredicate MakeMagPredicate_BoltAction(typename magWellType) {
        SCR_MagazinePredicate predicate = new SCR_MagazinePredicate();
        predicate.magWellType = magWellType;
        return predicate;
    }

    private IEntity FindMatchingMagInInventory_BoltAction() {
        SCR_InventoryStorageManagerComponent invMan = GetInvMan_BoltAction();
        if(!invMan)
            return null;

        typename magWellType = GetMagWellType_BoltAction();
        SCR_MagazinePredicate predicate = MakeMagPredicate_BoltAction(magWellType);

        return invMan.FindItem(predicate);
    }

    private EntitySlotInfo GetLeftHandSlot_BoltAction() {
        return GetLeftHandPointInfo();
    }
	
	private bool IsLocalPlayer_BoltAction(){
		PlayerController playerController = GetGame().GetPlayerController();
		if (!playerController)
			return false;
		IEntity playerControllerEnt = playerController.GetControlledEntity();
		if (!playerControllerEnt)
			return false;	
		IEntity weapon = GetWeaponEnt_BoltAction();
		if (!weapon)
			return false;
		IEntity characterEnt = GetOwner();
		if (!characterEnt)
			return false;

		return playerControllerEnt == characterEnt;
	}
};

