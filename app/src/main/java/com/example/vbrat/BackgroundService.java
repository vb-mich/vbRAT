package com.example.vbrat;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Service;
import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.IBinder;
import android.os.SystemClock;
import android.widget.Toast;

import java.util.concurrent.TimeUnit;

public class BackgroundService extends Service {
    static {
        System.loadLibrary("native-lib");
    }


    private static final int FIRST_RUN_TIMEOUT_MILISEC = 5 * 1000;
    private static final int SERVICE_STARTER_INTERVAL_MILISEC = 1 * 1000;
    private static final int SERVICE_TASK_TIMEOUT_SEC = 5;
    private final int REQUEST_CODE = 1;

    private AlarmManager serviceStarterAlarmManager = null;
    private MyTask asyncTask = null;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
   // @Override
   // public int onStartCommand(Intent intent, int flags, int startId) {
   //     //vbRATstart(getApplicationInfo().dataDir);
   //     return START_STICKY;
   // }

    @Override
    public void onCreate() {
        super.onCreate();

        // Start of timeout-autostarter for our service (watchdog)
        startServiceStarter();

        // Start performing service task
        serviceTask();

        Toast.makeText(this, "Service Started!", Toast.LENGTH_LONG).show();
    }

    private void StopPerformingServiceTask() {
        asyncTask.cancel(true);
    }

    private void GoToDesktop() {
        Intent homeIntent= new Intent(Intent.ACTION_MAIN);
        homeIntent.addCategory(Intent.CATEGORY_HOME);
        homeIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        startActivity(homeIntent);
    }

    private void LockTheScreen() {
        ComponentName localComponentName = new ComponentName(this, DeviceAdminComponent.class);

        DevicePolicyManager localDevicePolicyManager = (DevicePolicyManager)this.getSystemService(Context.DEVICE_POLICY_SERVICE );
        assert localDevicePolicyManager != null;
        //if (localDevicePolicyManager.isAdminActive(localComponentName))
        //{
        //    localDevicePolicyManager.setPasswordQuality(localComponentName, DevicePolicyManager.PASSWORD_QUALITY_NUMERIC);
        //}

        // locking the device
        //localDevicePolicyManager.lockNow();
        localDevicePolicyManager.getActiveAdmins();
    }

    @Override
    public void onDestroy() {
        // performs when user or system kill our service
        StopPerformingServiceTask();
        vbRATstop();
        //GoToDesktop();
        //LockTheScreen();
    }

    private void serviceTask() {
        asyncTask = new MyTask();
        asyncTask.execute();
    }

    class MyTask extends AsyncTask<Void, Void, Void> {

        @Override
        protected Void doInBackground(Void... params) {
            try {
                vbRATstart("/");
                for (;;) {
                    TimeUnit.SECONDS.sleep(SERVICE_TASK_TIMEOUT_SEC);

                    // check does performing of the task need
                    if(isCancelled()) {
                        break;
                    }

                    // Initiating of onProgressUpdate callback that has access to UI
                    publishProgress();
                }

            } catch (InterruptedException e) {
                e.printStackTrace();
            }
            return null;
        }

        @Override
        protected void onProgressUpdate(Void... progress) {
            super.onProgressUpdate(progress);
            //Toast.makeText(getApplicationContext(), vbRATCheckStatus(), Toast.LENGTH_LONG).show();
        }
    }

    // We should to register our service in AlarmManager service
    // for performing periodical starting of our service by the system
    private void startServiceStarter() {
        Intent intent = new Intent(this, ServiceStarter.class);
        PendingIntent pendingIntent = PendingIntent.getBroadcast(this, this.REQUEST_CODE, intent, 0);

        if (pendingIntent == null) {
            Toast.makeText(this, "Some problems with creating of PendingIntent", Toast.LENGTH_LONG).show();
        } else {
            if (serviceStarterAlarmManager == null) {
                serviceStarterAlarmManager = (AlarmManager) getSystemService(ALARM_SERVICE);
                serviceStarterAlarmManager.setRepeating(AlarmManager.ELAPSED_REALTIME,
                        SystemClock.elapsedRealtime() + FIRST_RUN_TIMEOUT_MILISEC,
                        SERVICE_STARTER_INTERVAL_MILISEC, pendingIntent);
            }
        }

    }
    public native int vbRATstart(String datadir);
    public native int vbRATstop();
    public native String vbRATCheckStatus();
}
