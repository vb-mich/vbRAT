package com.example.vbrat;

import android.os.Build;
import androidx.core.app.ActivityCompat;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import android.os.Handler;

public class MainActivity extends AppCompatActivity {

    static {
        System.loadLibrary("native-lib");
    }

    //private TextView tv;
    private int cont = 0;


    public void startev(int nn) {
        TextView tv = findViewById(R.id.sample_text);

        tv.setText(checkStatus(0));
        Handler handler = new Handler();
        handler.postDelayed(new Runnable() {
            public void run() {
                startev(cont);
            }
        }, 500);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {

        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        //checkStatus(2);
        Handler handler = new Handler();
        vbRATstart();
        handler.postDelayed(new Runnable() {
            public void run() {
                startev(cont);
            }
        }, 1000);

    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native String checkStatus(int what);
    public native int vbRATstart();
}
