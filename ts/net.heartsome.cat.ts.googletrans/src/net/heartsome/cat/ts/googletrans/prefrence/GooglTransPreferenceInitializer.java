/**
 * GooglTransPreferenceInitializer.java
 *
 * Version information :
 *
 * Date:2012-5-13
 *
 * Copyright notice :
 * 本文件及其附带的相关文件包含机密信息，仅限瀚特盛科技有限公司指定的，与本项目有关的内部人员和客户联络人员查阅和使用。 
 * 如果您不是本保密声明中指定的收件者，请立即销毁本文件，禁止对本文件或根据本文件中的内容采取任何其他行动， 
 * 包括但不限于：泄露本文件中的信息、以任何方式制作本文件全部或部分内容之副本、将本文件或其相关副本提供给任何其他人。
 */
package net.heartsome.cat.ts.googletrans.prefrence;

import net.heartsome.cat.ts.googletrans.Activator;
import net.heartsome.cat.ts.googletrans.bean.IPreferenceConstant;

import org.eclipse.core.runtime.preferences.AbstractPreferenceInitializer;
import org.eclipse.jface.preference.IPreferenceStore;

/**
 * @author  jason
 * @version 
 * @since   JDK1.6
 */
public class GooglTransPreferenceInitializer extends AbstractPreferenceInitializer {


	/** (non-Javadoc)
	 * @see org.eclipse.core.runtime.preferences.AbstractPreferenceInitializer#initializeDefaultPreferences()
	 */
	@Override
	public void initializeDefaultPreferences() {
		IPreferenceStore ps = Activator.getDefault().getPreferenceStore();
		ps.setDefault(IPreferenceConstant.STATE, false);
		ps.setDefault(IPreferenceConstant.KEY, "");
		
		ps.setDefault(IPreferenceConstant.NO_REPEATE_ACCESS, true);
		ps.setDefault(IPreferenceConstant.ALWAYS_ACCESS, false);
		ps.setDefault(IPreferenceConstant.MANUAL_ACCESS, false);
		
		ps.setDefault(IPreferenceConstant.PRETRANS_STATE, false);
	}

}
