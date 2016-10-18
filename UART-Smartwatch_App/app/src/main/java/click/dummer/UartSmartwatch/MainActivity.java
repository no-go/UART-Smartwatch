/*
 * Copyright (c) 2015, Nordic Semiconductor
 * All rights reserved.
 * 
 * 2016 - modified many parts by Jochen Peters
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
package click.dummer.UartSmartwatch;

import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.app.Activity;
import android.app.AlertDialog;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.content.res.Configuration;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.preference.PreferenceManager;
import android.support.v4.content.LocalBroadcastManager;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {
    public static final String TAG = MainActivity.class.getSimpleName();
    private Context ctx;

    private static final int WATCH_REQUEST = 1;
    private static final int REQUEST_ENABLE_BT = 2;

    private NotificationReceiver nReceiver;
    private UartService mService = null;
    private BluetoothDevice mDevice = null;
    private BluetoothAdapter mBtAdapter = null;
    private TextView listMessage;
    private Button btnConnectDisconnect, btnSend;
    private EditText edtMessage;
    private SharedPreferences mPreferences;
    private String devAddr = PreferencesActivity.DEFAULT_ADDR;

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            if (msg.what == WATCH_REQUEST) {
                Date dNow = new Date();
                String dateFormat = mPreferences.getString("dateFormat", "'#'HH:mm:ss");
                SimpleDateFormat ft =
                        new SimpleDateFormat(dateFormat);
                String timeStr = ft.format(dNow);

                if (btnSend.isEnabled()) {
                    sendMsg(timeStr  + myNewLine(timeStr) + listMessage.getText().toString());
                }
            }
        }
    };

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        menu.add(R.string.preferences);
        MenuItem item = menu.getItem(0);
        item.setShowAsAction(MenuItem.SHOW_AS_ACTION_ALWAYS);
        item.setIcon(android.R.drawable.ic_menu_preferences);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        if(item.getTitle() == getString(R.string.preferences)) {
            Intent intent = new Intent(MainActivity.this, PreferencesActivity.class);
            startActivity(intent);
        }
        return true;
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        ctx = this;
        mPreferences = PreferenceManager.getDefaultSharedPreferences(this);

        setContentView(R.layout.main);
        mBtAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBtAdapter == null) {
            Toast.makeText(this, "Bluetooth is not available", Toast.LENGTH_LONG).show();
            finish();
            return;
        }


        // ----------------------------------------------------------
        if (mBtAdapter.isEnabled() && mBtAdapter.isDiscovering()) {
            mBtAdapter.cancelDiscovery();
        }
        mBtAdapter.startDiscovery();
        // ----------------------------------------------------------


        btnConnectDisconnect = (Button) findViewById(R.id.btn_select);
        btnSend = (Button) findViewById(R.id.sendButton);
        edtMessage = (EditText) findViewById(R.id.sendText);
        listMessage = (TextView) findViewById(R.id.listMessage);

        service_init();

        // Handle Disconnect & Connect button
        btnConnectDisconnect.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                if (mBtAdapter.isEnabled()) {
                    if (btnConnectDisconnect.getText().equals(getString(R.string.connect))) {

                        mDevice = mBtAdapter.getRemoteDevice(devAddr);
                        ((TextView) findViewById(R.id.rssival)).setText("'" + devAddr + "' - Connecting");
                        mService.connect(devAddr);
                    } else {
                        //Disconnect button pressed
                        if (mDevice != null) {
                            mService.disconnect();
                        }
                    }
                }
            }
        });

        btnSend.setOnClickListener(new View.OnClickListener() {
            // --------------------------------------------------------------------------------------------------------
            @Override
            public void onClick(View v) {
                String message = edtMessage.getText().toString().trim();
                int messageSize = Integer.parseInt(mPreferences.getString("messageSize", "200"));
                if (message.length() > messageSize) message = message.substring(0, messageSize);
                listMessage.setText(message);
                sendMsg(message);
                edtMessage.setText("");
            }
        });

        nReceiver = new NotificationReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction("click.dummer.UartNotify.NOTIFICATION_LISTENER");
        registerReceiver(nReceiver, filter);
    }

    public void sendMsg(String message) {
        message = message.replaceAll("„","\"");
        message = message.replaceAll("“","\"");
        message = message.replaceAll("[\\x{1F643}]","(: ");
        message = message.replaceAll("[\\x{1F61B}]",":-} ");
        message = message.replaceAll("[\\x{1F60E}]","B-) ");
        message = message.replaceAll("[\\x{1F913}]","B-) ");
        message = message.replaceAll("[\\x{1F61D}]","X-P ");
        message = message.replaceAll("[\\x{1F609}]",";-) ");
        message = message.replaceAll("[\\x{1F60B}]",";-9 ");
        message = message.replaceAll("[\\x{1F642}]",":-) ");
        message = message.replaceAll("[\\x{1F601}]","E-) ");
        message = message.replaceAll("[\\x{1F604}]","E-) ");
        message = message.replaceAll("[\\x{1F603}]",":-D ");
        message = message.replaceAll("[\\x{1F600}]",":-D ");
        message = message.replaceAll("[\\x{1F606}]","X-D ");
        message = message.replaceAll("[\\x{1F644}]","@@ "); // roll eyes
        message = message.replaceAll("[\\x{1F602}]","=D ");
        message = message.replaceAll("[\\x{1F605}]","=) ");
        message = message.replaceAll("[\\x{1F62D}]","X-( ");
        message = message.replaceAll("[\\x{1F633}]",":o) ");

        // catch the rest of emoji
        Pattern unicodeOutliers = Pattern.compile("\\p{InEmoticons}");
        Matcher unicodeOutlierMatcher = unicodeOutliers.matcher(message);
        message = unicodeOutlierMatcher.replaceAll("\2");

        byte[] value;
        try {
            value = (message + "\n").getBytes("UTF-8");
            mService.writeRXCharacteristic(value);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private ServiceConnection mServiceConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName className, IBinder rawBinder) {
            mService = ((UartService.LocalBinder) rawBinder).getService();
            Log.d(TAG, "onServiceConnected mService= " + mService);
            if (!mService.initialize()) {
                Log.e(TAG, "Unable to initialize Bluetooth");
                finish();
            }

        }

        public void onServiceDisconnected(ComponentName classname) {
            mService = null;
        }
    };

    private final BroadcastReceiver UARTStatusChangeReceiver = new BroadcastReceiver() {
        String action;

        public void onReceive(Context context, Intent intent) {
            action = intent.getAction();

            if (action.equals(UartService.DEVICE_DOES_NOT_SUPPORT_UART)) {
                showMessage("Device doesn't support UART. Disconnecting");
                mService.disconnect();
            }

            if (action.equals(UartService.ACTION_GATT_SERVICES_DISCOVERED)) {
                mService.enableTXNotification();
            }

            if (action.equals(UartService.ACTION_GATT_CONNECTED)) {
                runOnUiThread(new Runnable() {
                    public void run() {
                        btnConnectDisconnect.setText(R.string.disconnect);
                        edtMessage.setEnabled(true);
                        btnSend.setEnabled(true);
                        String name = mDevice.getName();
                        if (name == null) name = mDevice.getAddress();
                        ((TextView) findViewById(R.id.rssival)).setText(
                                "'" + name  + "' - Ready");
                    }
                });
            }

            if (action.equals(UartService.ACTION_GATT_DISCONNECTED)) {
                runOnUiThread(new Runnable() {
                    public void run() {
                        btnConnectDisconnect.setText(R.string.connect);
                        edtMessage.setEnabled(false);
                        btnSend.setEnabled(false);
                        ((TextView) findViewById(R.id.rssival)).setText(R.string.notConnected);
                        mService.close();

                        // try reconnect
                        new Handler().postDelayed(new Runnable() {
                            public void run() {
                                if (action.equals(UartService.ACTION_GATT_DISCONNECTED)) {
                                    ((TextView) findViewById(R.id.rssival)).setText("Try reconnect ...");
                                    btnConnectDisconnect.callOnClick();
                                }
                            }
                        }, 10000);
                    }
                });
            }

            if (action.equals(UartService.ACTION_DATA_AVAILABLE)) {
                final byte[] txValue = intent.getByteArrayExtra(UartService.EXTRA_DATA);
                runOnUiThread(new Runnable() {
                    public void run() {
                        try {
                            //String text = new String(txValue, "UTF-8");
                            String timeRequest = mPreferences.getString("timeRequest", "~");
                            if (txValue[0] == timeRequest.charAt(0)) {
                                mHandler.sendEmptyMessage(WATCH_REQUEST);
                            }
                        } catch (Exception e) {
                            Log.e(TAG, e.toString());
                        }
                    }
                });
            }

        }
    };

    private void service_init() {
        Intent bindIntent = new Intent(this, UartService.class);
        bindService(bindIntent, mServiceConnection, Context.BIND_AUTO_CREATE);

        LocalBroadcastManager.getInstance(this).registerReceiver(UARTStatusChangeReceiver, makeGattUpdateIntentFilter());
    }

    private static IntentFilter makeGattUpdateIntentFilter() {
        final IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(UartService.ACTION_GATT_CONNECTED);
        intentFilter.addAction(UartService.ACTION_GATT_DISCONNECTED);
        intentFilter.addAction(UartService.ACTION_GATT_SERVICES_DISCOVERED);
        intentFilter.addAction(UartService.ACTION_DATA_AVAILABLE);
        intentFilter.addAction(UartService.DEVICE_DOES_NOT_SUPPORT_UART);
        return intentFilter;
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        LocalBroadcastManager.getInstance(this).unregisterReceiver(UARTStatusChangeReceiver);
    }

    @Override
    public void onResume() {
        devAddr = mPreferences.getString("ble_addr", PreferencesActivity.DEFAULT_ADDR);
        super.onResume();
        if (!mBtAdapter.isEnabled()) {
            Log.i(TAG, "onResume - BT not enabled yet");
            Intent enableIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableIntent, REQUEST_ENABLE_BT);
        }

    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
    }

    private void showMessage(String msg) {
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show();
    }

    @Override
    public void onBackPressed() {
        new AlertDialog.Builder(this)
            .setIcon(android.R.drawable.ic_dialog_alert)
            .setTitle(R.string.Quit)
            .setMessage(R.string.reallyQuit)
            .setPositiveButton(R.string.Yes, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    LocalBroadcastManager.getInstance(ctx).unregisterReceiver(UARTStatusChangeReceiver);
                    unbindService(mServiceConnection);
                    unregisterReceiver(nReceiver);
                    mService.disconnect();
                    mService.close();
                    mService.stopSelf();
                    mService = null;
                    finish();
                }
            })
            .setNegativeButton(R.string.No, null)
            .show();
    }

    private String myNewLine(String s) {
        return "/";
    }

    class NotificationReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (BluetoothDevice.ACTION_FOUND.equals(action)){
                // Get the BluetoothDevice object from the Intent
                BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                if (device.getAddress().equals(devAddr)) {
                    mBtAdapter.cancelDiscovery();
                }
            }

            if (intent.getStringExtra("MSG") != null) {

                String temp = intent.getStringExtra("MSG").trim();
                int messageSize = Integer.parseInt(mPreferences.getString("messageSize", "120"));
                if (btnSend.isEnabled()) {
                    temp = temp + myNewLine(temp) + listMessage.getText().toString().trim();
                    if (temp.length() > messageSize) {
                        temp = temp.substring(0, messageSize);
                    }
                    listMessage.setText(temp);
                }
            }
        }
    }
}
