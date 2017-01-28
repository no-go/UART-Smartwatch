/*
 * The MIT License (MIT)

Copyright (c) 2016 Jochen Peters (JotPe, Krefeld)

Permission is hereby granted, free of charge, to any person obtaining 
a copy of this software and associated documentation files (the "Software"), 
to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, 
and/or sell copies of the Software, and to permit persons to whom the 
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included 
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
DEALINGS IN THE SOFTWARE.
*/
package click.dummer.UartSmartwatch;

import android.app.Notification;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Bundle;
import android.preference.PreferenceManager;
import android.service.notification.NotificationListenerService;
import android.service.notification.StatusBarNotification;
import android.graphics.Color;
import android.util.Log;

import java.util.ArrayList;

public class NotificationService extends NotificationListenerService {
    public static final int DEFAULT_COLOR = 0xffddff88;
    private SharedPreferences mPreferences;
    private String lastPost = "";
    private String lastTitle = "";

    @Override
    public void onNotificationPosted(StatusBarNotification sbn) {
        Notification noti = sbn.getNotification();
        Bundle extras = noti.extras;
        String title = extras.getString(Notification.EXTRA_TITLE);
        String pack = sbn.getPackageName();
        String msg = (String) noti.tickerText;
        String msg2 = extras.getString(Notification.EXTRA_TEXT);
        String msg3 = null;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            msg3 = extras.getString(Notification.EXTRA_BIG_TEXT);
        }

        if (noti.ledARGB == 0) noti.ledARGB = DEFAULT_COLOR;

        ArrayList<Integer> rgb = new ArrayList<Integer>(3);
        rgb.add(Color.red(noti.ledARGB));
        rgb.add(Color.green(noti.ledARGB));
        rgb.add(Color.blue(noti.ledARGB));

        try {
            Log.d(MainActivity.TAG, title);
            Log.d(MainActivity.TAG, getPackageName());
            Log.d(MainActivity.TAG, msg);
            Log.d(MainActivity.TAG, " "+msg2);
            Log.d(MainActivity.TAG, " "+msg3);
            Log.d(MainActivity.TAG, "RGB: " + rgb.get(0) + " " +rgb.get(1) + " " + rgb.get(2) );
        } catch (Exception e) {}

        //if (msg2 != null && msg2.length()>0) msg = msg2;
        //if (msg3 != null && msg3.length()>0) msg = msg3;

        // catch not normal message .-----------------------------
        if (!sbn.isClearable()) return;
        /*if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            if (sbn.isGroup()) {
                Log.d(MainActivity.TAG, "is group");
                return;
            }
        }*/
        if (msg == null) return;
        if (msg.equals(lastPost) ) return;
        if (title.equals(lastTitle) ) msg = msg.replaceFirst(title, "");

        lastPost  = msg;
        lastTitle = title;
        //--------------------------------------------------------

        mPreferences = PreferenceManager.getDefaultSharedPreferences(getApplicationContext());
        int messageLimit = Integer.parseInt(mPreferences.getString("messageLimit", "100"));

        Intent i = new  Intent("click.dummer.UartNotify.NOTIFICATION_LISTENER");
        if (msg.length() > messageLimit) {
            msg = title;
        }
        i.putExtra("MSG", msg);
        i.putExtra("posted", true);
        i.putIntegerArrayListExtra("RGB", rgb);
        i.putExtra("App", pack);
        i.putExtra("delayOn", noti.ledOnMS);
        i.putExtra("delayOff", noti.ledOffMS);
        sendBroadcast(i);
    }

    @Override
    public void onNotificationRemoved(StatusBarNotification sbn) {
        Notification noti = sbn.getNotification();
        Bundle extras = noti.extras;
        String title = extras.getString(Notification.EXTRA_TITLE);
        String msg = (String) noti.tickerText;
        String pack = sbn.getPackageName();

        // catch not normal message .-----------------------------
        if (!sbn.isClearable()) return;
        /*if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            if (sbn.isGroup()) {
                Log.d(MainActivity.TAG, "is group");
                return;
            }
        }*/
        if (msg == null) return;

        if (title.equals(lastTitle) ) msg = msg.replaceFirst(title, "");

        lastPost  = msg;
        lastTitle = title;
        //--------------------------------------------------------

        Intent i = new  Intent("click.dummer.UartNotify.NOTIFICATION_LISTENER");
        i.putExtra("MSG", "notify removed");
        i.putExtra("posted", false);
        i.putExtra("App", pack);
        sendBroadcast(i);
    }
}
