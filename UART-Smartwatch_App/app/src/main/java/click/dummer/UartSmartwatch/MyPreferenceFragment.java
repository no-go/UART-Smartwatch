package click.dummer.UartSmartwatch;

import android.os.Bundle;
import android.preference.PreferenceFragment;

public class MyPreferenceFragment extends PreferenceFragment {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        addPreferencesFromResource(R.xml.preferences);
    }
}