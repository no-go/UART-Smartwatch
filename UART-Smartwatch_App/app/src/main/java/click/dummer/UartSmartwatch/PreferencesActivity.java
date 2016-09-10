package click.dummer.UartSmartwatch;

import android.app.Activity;
import android.os.Bundle;
import android.support.v4.app.NavUtils;
import android.view.MenuItem;

public class PreferencesActivity extends Activity {
    public static final String DEFAULT_ADDR = "DE:D2:4A:F4:F9:47";

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        switch (item.getItemId()) {
            case android.R.id.home:
                NavUtils.navigateUpFromSameTask(this);
                return true;
            default:
                return super.onOptionsItemSelected(item);
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.pref_main);
        getActionBar().setTitle(R.string.preferences);
        getActionBar().setDisplayHomeAsUpEnabled(true);
    }
}
