modded class SCR_CharacterCommandHandlerComponent : CharacterCommandHandlerComponent
{
	protected bool m_bWasPullingTrigger_BC = false;
	// For handling the click "pumping" on the bolt guns
	override bool HandleWeaponFire(CharacterInputContext pInputCtx, float pDt, int pCurrentCommandID) {
		bool isPullingTrigger_BC = pInputCtx.WeaponIsPullingTrigger();
		
		if (isPullingTrigger_BC && !m_bWasPullingTrigger_BC) {
			m_bWasPullingTrigger_BC = true;
			if (m_WeaponManager) {
				BaseWeaponComponent currentWeapon = m_WeaponManager.GetCurrentWeapon();
				if (currentWeapon) {
					IEntity weaponEntity = currentWeapon.GetOwner();
					BC_BoltAnimationComponent boltAnimComp = BC_BoltAnimationComponent.Cast(weaponEntity.FindComponent(BC_BoltAnimationComponent));
					if (boltAnimComp)
						boltAnimComp.TryRackBolt();
				}
			}
		}
		else if (!isPullingTrigger_BC && m_bWasPullingTrigger_BC)
			m_bWasPullingTrigger_BC = false;
	
		return HandleWeaponFireDefault(pInputCtx, pDt, pCurrentCommandID);
	}
}
