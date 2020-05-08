package com.example.vbrat;
import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

public class service extends Service {
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        vbRATstart(getApplicationInfo().dataDir);
        return START_STICKY;
    }
    @Override
    public void onDestroy() {
        super.onDestroy();
        vbRATstop();
    }

    public native int vbRATstart(String datadir);
    public native int vbRATstop();
}

