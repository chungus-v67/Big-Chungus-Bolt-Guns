class SCR_BoltAnimationComponentClass : WeaponAnimationComponentClass
{
}

class SCR_BoltAnimationComponent : WeaponAnimationComponent
{
	[Attribute("", UIWidgets.ResourceNamePicker, desc: "The particle effect that you want to play when the bolt is cycled", params: "ptc")]
	protected ResourceName m_sCasingEjectionAnimation;
	
	[Attribute(desc: "Effect position")]
	protected ref PointInfo		m_effectPosition;											//the position of the m_effectPosition
	
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
	protected IEntity m_player;
	protected AnimationEventID m_setAmmoCount = -1;
	protected AnimationEventID m_getAmmoCount = -1;
	protected AnimationEventID m_checkMag = -1;
	protected AnimationEventID m_ejectRound = -1;
	protected AnimationEventID m_spawnRound = -1;
	protected AnimationEventID m_despawnRound = -1;
	protected AnimationEventID m_adjustAmmoCount = -1;
	protected AnimationEventID m_emptyMagAfterReloadCheck = -1;
	protected AnimationEventID m_closeBolt = -1;
	protected AnimationEventID m_startReloadTimer = -1;
	protected TAnimGraphVariable m_animAmmoCount = -1;
	protected TAnimGraphVariable m_animStopReloading = -1;
	protected TAnimGraphVariable m_animCloseBoltFlag = -1;
	protected TAnimGraphVariable m_playerAnimStopReloading = -1;
	protected TAnimGraphVariable m_playerAmmoCount = -1;
	protected TAnimGraphVariable m_playerAnimCloseBoltFlag = -1;
	protected typename magWellType;
	protected int currentAmmoCount = 0;
	protected float m_startTime = -1;
	IEntity dummyMag = null;
	protected bool wasMagEmpty = false;
	protected bool wasMagEmptyAfterReload = false;
	protected int meshIdx = -1;
	IEntity mosinRound = null;
	protected int magSlotIndex = -1;
	protected MagazineComponent defaultMagComp = null;
	protected int count = 0;
	protected BaseMagazineComponent magazine = null;
	
	void SCR_BoltAnimationComponent(IEntityComponentSource src, IEntity ent, IEntity parent)
	{
		m_Owner = ent;
		m_player = parent;
		m_animAmmoCount = BindIntVariable("MosinAmmoCount");
		m_animStopReloading = BindBoolVariable("MosinStopReloading");
		m_animCloseBoltFlag = BindBoolVariable("MosinCloseBoltFlag");
		m_setAmmoCount = GameAnimationUtils.RegisterAnimationEvent("SetAmmoCount");
		m_adjustAmmoCount = GameAnimationUtils.RegisterAnimationEvent("AdjustAmmoCount");
		m_getAmmoCount = GameAnimationUtils.RegisterAnimationEvent("GetAmmoCount");
		m_checkMag = GameAnimationUtils.RegisterAnimationEvent("CheckMag"); 
		m_ejectRound = GameAnimationUtils.RegisterAnimationEvent("EjectRound");
		m_spawnRound = GameAnimationUtils.RegisterAnimationEvent("SpawnRound");
		m_despawnRound = GameAnimationUtils.RegisterAnimationEvent("DespawnRound");
		m_emptyMagAfterReloadCheck = GameAnimationUtils.RegisterAnimationEvent("EmptyMagAfterReloadCheck");
		m_closeBolt = GameAnimationUtils.RegisterAnimationEvent("CloseBolt");
		m_startReloadTimer = GameAnimationUtils.RegisterAnimationEvent("StartReloadTimer");
	}
	
	
	override event void OnAnimationEvent(AnimationEventID animEventType, AnimationEventID animUserString, int intParam, float timeFromStart, float timeToEnd)
	{
		super.OnAnimationEvent(animEventType, animUserString, intParam, timeFromStart, timeToEnd);		
		

		protected WeaponComponent weapon = WeaponComponent.Cast(m_Owner.FindComponent(WeaponComponent));	
		ChimeraCharacter character = ChimeraCharacter.Cast(m_Owner.GetParent());		
		protected SCR_CharacterControllerComponent controller = SCR_CharacterControllerComponent.Cast(character.GetCharacterController());					
			
		protected CharacterAnimationComponent animation = CharacterAnimationComponent.Cast(controller.GetOwner().FindComponent(CharacterAnimationComponent));		
		GroupInfoDisplay test = GroupInfoDisplay.Cast(character.GetInfoDisplay());		
		protected SCR_WeaponInfo hud = SCR_WeaponInfo.Cast(character.GetInfoDisplay());
		protected SCR_InventoryStorageManagerComponent invMan = SCR_InventoryStorageManagerComponent.Cast(controller.GetInventoryStorageManager());	
		if (!magazine){ 
			magazine = MagazineComponent.Cast(weapon.GetCurrentMagazine());	
		}
		
		if (magazine && magSlotIndex == -1){
			InventoryMagazineComponent magInvComp = InventoryMagazineComponent.Cast(magazine.GetOwner().FindComponent(InventoryMagazineComponent));
			if (magInvComp) {
				magSlotIndex = magInvComp.GetParentSlot().GetID();
			}
		}
		
		// Init variables
		// *************************************************************************
		
		if (meshIdx == -1) {
			meshIdx = GameAnimationUtils.FindMeshIndex(weapon.GetOwner(), "Cartridge");
		} 
	
		m_playerAmmoCount = animation.BindVariableInt("MosinAmmoCount");
		m_playerAnimStopReloading = animation.BindVariableBool("MosinStopReloading");
		m_playerAnimCloseBoltFlag = animation.BindVariableBool("MosinCloseBoltFlag");
		m_playerAnimStopReloading = animation.BindVariableBool("MosinStopReloading");

		
		if (!magWellType) {
			magWellType.Spawn();
			if (magazine){
				magWellType = magazine.GetMagazineWell().Type();
			}
		}
		// ***************************************************************************
		
		
		// Event that increments ammo count and stores new value to animation variable
		if (animEventType == m_setAmmoCount) {
			currentAmmoCount +=1;
			//shows the "cartridge" mesh in the gun. This is not the real magazine, just a part of the gun's mesh
			GameAnimationUtils.ShowMesh(weapon.GetOwner(),meshIdx,true); 
			
			if (magazine){
				magazine.SetAmmoCount(currentAmmoCount);			
				if (currentAmmoCount >= m_sMaxAmmo){
					animation.SetVariableInt(m_playerAnimStopReloading, true);
					SetIntVariable(m_animStopReloading, true);	 
				} 
			} 
		}
		
	
		//checks for more mags in the inventory. Used to set the MosinStopReloading flag when there are no mags left in inventory
		if (animEventType == m_checkMag) {
			protected SCR_MagazinePredicate predicate = new SCR_MagazinePredicate();	
			predicate.magWellType = magWellType;
			protected IEntity newMag = invMan.FindItem(predicate);			
			if (newMag == null){
				animation.SetVariableInt(m_playerAnimStopReloading, true);
				SetBoolVariable(m_animStopReloading, true);
			} 			
			
			if (magazine){
				currentAmmoCount = magazine.GetAmmoCount();	
				if (currentAmmoCount >= m_sMaxAmmo || newMag == null){
					animation.SetVariableInt(m_playerAnimStopReloading, true);
					SetIntVariable(m_animStopReloading, true);
				}
			}
			
			Print(GetGame().GetWorld().GetWorldTime() - m_startTime);
			// If the time exceeds the amount set, this attempts to exit the reload as remediation
			if (m_startTime != -1 && GetGame().GetWorld().GetWorldTime() - m_startTime > m_sMaxReloadTime ){
				animation.SetVariableInt(m_playerAnimStopReloading, true);
				SetIntVariable(m_animStopReloading, true);
				if(magazine) {
					magazine.SetAmmoCount(m_sMaxAmmo);
				}
				m_startTime = -1;
			}
			
			animation.SetVariableBool(m_playerAnimCloseBoltFlag, false);
			SetBoolVariable(m_animCloseBoltFlag, false);	
		}
		
		// Grabs a magazine from the inventory and attaches it to the left hand prop for the animation
		if (animEventType == m_spawnRound){
			protected SCR_MagazinePredicate predicate = new SCR_MagazinePredicate();
			predicate.magWellType = magWellType;
			Resource prefab = Resource.Load(m_sShellPrefab);
			EntitySpawnParams spawnParams3 = new EntitySpawnParams();
			mosinRound = GetGame().SpawnEntityPrefab(prefab, GetGame().GetWorld());
			EntitySlotInfo leftHandSlot = controller.GetLeftHandPointInfo();
			leftHandSlot.AttachEntity(mosinRound);
		}
		
		// Despawns the magazine from the left hand and deletes it
		if (animEventType == m_despawnRound){
			if (mosinRound){
				EntitySlotInfo leftHandSlot = controller.GetLeftHandPointInfo();
				leftHandSlot.AttachEntity(mosinRound);
				leftHandSlot.DetachEntity();
				RplComponent.DeleteRplEntity(mosinRound, false);
				mosinRound = null;
			} else {
				animation.SetVariableInt(m_playerAnimStopReloading, true);
				SetIntVariable(m_animStopReloading, true);
			}
			
			protected SCR_MagazinePredicate predicate = new SCR_MagazinePredicate();	
			predicate.magWellType = magWellType;
			protected IEntity newMag = invMan.FindItem(predicate);	

			if (newMag){
				RplComponent.DeleteRplEntity(newMag, false);
			} 
			else {
				animation.SetVariableInt(m_playerAnimStopReloading, true);
				SetIntVariable(m_animStopReloading, true);
			}
			
			animation.SetVariableBool(m_playerAnimCloseBoltFlag, false);
			SetBoolVariable(m_animCloseBoltFlag, false);
								

		}		
		
		// Called on fire animation, before the bolt is racked or reload starts. Used to make adjustments to the ammo count
		if (animEventType == m_adjustAmmoCount) {			
			// make sure the real mag in the gun always hidden. 
			if (magazine){
				if (m_sCasingMesh){GameAnimationUtils.ShowMesh(magazine.GetOwner(),GameAnimationUtils.FindMeshIndex(magazine.GetOwner(), m_sCasingMesh),false)};
				if (m_sBulletMesh){GameAnimationUtils.ShowMesh(magazine.GetOwner(),GameAnimationUtils.FindMeshIndex(magazine.GetOwner(), m_sBulletMesh),false)};
			}
			// removes the extra round that was added to the mag in the case of only one round being reloaded.
			if (wasMagEmptyAfterReload == true) {
				magazine.SetAmmoCount(0);
				wasMagEmptyAfterReload = false;

			}
			//hide mag if empty
			if (magazine){
				if (magazine.GetAmmoCount() == 0){
					GameAnimationUtils.ShowMesh(weapon.GetOwner(),meshIdx,false);
				}
			}
		}
		
		if (animEventType == m_emptyMagAfterReloadCheck) {
			m_startTime = -1; //This needs to be called at the end of the reload after the loop. Just putting it here for simplicity
			
			// also calling this here to perform cleanup and prevent it from going into a bad state where it deletes the mag
			animation.SetVariableInt(m_playerAnimStopReloading, false);
			SetIntVariable(m_animStopReloading, false);		
			
			
			protected SCR_MagazinePredicate predicate = new SCR_MagazinePredicate();
			predicate.magWellType = magWellType;
			dummyMag = invMan.FindItem(predicate);	
			if (magazine) {
			
				if (magazine.GetAmmoCount() == 0 && dummyMag == null) {
					magazine.SetAmmoCount(1);
					wasMagEmptyAfterReload = true;	
				}
			}
		}
		

		// spawns the casing eject effect when the bolt is cycled
		if (animEventType == m_ejectRound) {
			if (m_sCasingEjectionAnimation.Length() >0){
				protected ParticleEffectEntitySpawnParams spawnParams();
				spawnParams.Parent = magazine.GetOwner();
				vector mat[4];
				spawnParams.PivotID = m_effectPosition.GetNodeId();
				m_effectPosition.GetTransform(mat);
				spawnParams.Transform[0]=mat[0];
				spawnParams.Transform[1]=mat[1];
				spawnParams.Transform[2]=mat[2];
				spawnParams.Transform[3]=mat[3];
				protected ResourceName casingParticleEffect = m_sCasingEjectionAnimation;
				if (casingParticleEffect) {
					ParticleEffectEntity.SpawnParticleEffect(casingParticleEffect,spawnParams); // issue
				}
			}
		}
		
		// this is used to transition to the close bolt state because the player and weapon animations 
		if (animEventType == m_closeBolt) {
			animation.SetVariableBool(m_playerAnimCloseBoltFlag, true);
			SetBoolVariable(m_animCloseBoltFlag, true);
		}
				
		// Used at the beginning of the reload to set the variable that tracks how long the reload has gone on for.
		if (animEventType == m_startReloadTimer) {
			m_startTime = GetGame().GetWorld().GetWorldTime();
			animation.SetVariableInt(m_playerAnimStopReloading, false);
			SetIntVariable(m_animStopReloading, false);		
			

			if (!weapon.GetCurrentMagazine()){
				MuzzleComponent thisMuzzle = MuzzleComponent.Cast(weapon.FindComponent(MuzzleComponent));
				Resource defaultMagResource = Resource.Load(thisMuzzle.GetDefaultMagazineOrProjectileName());
				IEntity defaultMag = GetGame().SpawnEntityPrefab(defaultMagResource, GetGame().GetWorld());
				BaseRplComponent rplComponent = BaseRplComponent.Cast(defaultMag.FindComponent(BaseRplComponent));
				SCR_WeaponAttachmentsStorageComponent storage = SCR_WeaponAttachmentsStorageComponent.Cast(weapon.GetOwner().FindComponent(SCR_WeaponAttachmentsStorageComponent));
				if (rplComponent)
				{
	 			   rplComponent.InsertToReplication();
				} 	
				
				invMan.EquipWeaponAttachment(defaultMag, character);
				if (weapon.GetCurrentMagazine() != null){return;}
				controller.ReloadWeapon();
				if (weapon.GetCurrentMagazine() != null){return;}
				SCR_WeaponAttachmentsStorageComponent weaponAttachment = SCR_WeaponAttachmentsStorageComponent.Cast(m_Owner.FindComponent(SCR_WeaponAttachmentsStorageComponent)); 	
				InventoryStorageSlot magSlot = weaponAttachment.GetSlot(0);
				controller.ReloadWeaponWith(defaultMag, false);
				if (weapon.GetCurrentMagazine() != null){return;}
				magSlot.AttachEntity(defaultMag);
				if (weapon.GetCurrentMagazine() != null){return;}
				magazine = MagazineComponent.Cast(defaultMag.FindComponent(MagazineComponent));
				invMan.TrySwapItemStorages(defaultMag, weaponAttachment.Get(0));		
			}
		}
	}
	
}