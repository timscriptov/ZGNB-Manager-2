package com.github.koston.preference;

import androidx.fragment.app.FragmentManager;
import androidx.preference.Preference;
import androidx.preference.PreferenceDialogFragmentCompat;
import androidx.preference.PreferenceFragmentCompat;

@SuppressWarnings("unused")
public abstract class ColorPreferenceFragmentCompat extends PreferenceFragmentCompat {

    @Override
    public void onDisplayPreferenceDialog(Preference preference) {
        PreferenceDialogFragmentCompat dialogFragment = null;
        FragmentManager manager = getParentFragmentManager();
        if (preference instanceof ColorPreference) {
            dialogFragment = ColorPickerDialog.newInstance(preference.getKey());
        }
        if (dialogFragment != null) {
            dialogFragment.setTargetFragment(this, 0);
            dialogFragment.show(manager, ColorPickerDialog.class.getName());
        } else {
            super.onDisplayPreferenceDialog(preference);
        }
    }
}
