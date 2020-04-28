package com.example.vbrat;

import android.os.Build;
import androidx.core.app.ActivityCompat;
import androidx.appcompat.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;
import android.os.Handler;

import org.java_websocket.client.WebSocketClient;
import org.java_websocket.handshake.ServerHandshake;


import java.net.URI;
import java.net.URISyntaxException;

public class MainActivity extends AppCompatActivity {
    WebSocketClient mWebSocketClient;

    private void connectWebSocket() {

        URI uri;
        try {
            uri = new URI("wss://connect.websocket.in/v3/69?apiKey=l7d6CdYNyjLFsRA0uzyFD6Ec0pcPkhKFlYVNwJPeWgTmAIFhZoeM9U5LO3Zi");
        } catch (URISyntaxException e) {
            e.printStackTrace();
            return;
        }

        mWebSocketClient = new WebSocketClient(uri) {
            @Override
            public void onOpen(ServerHandshake serverHandshake) {
                Log.i("Websocket", "Opened");
                mWebSocketClient.send("Hello from " + Build.MANUFACTURER + " " + Build.MODEL);
            }

            @Override
            public void onMessage(String s) {
                final String message = s;
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        TextView textView = (TextView) findViewById(R.id.sample_text);
                        textView.setText(textView.getText() + "\n" + message);
                        mWebSocketClient.send(message);
                    }
                });
            }

            @Override
            public void onClose(int i, String s, boolean b) {
                Log.i("Websocket", "Closed " + s);
            }

            @Override
            public void onError(Exception e) {
                Log.i("Websocket", "Error " + e.getMessage());
            }
        };
        mWebSocketClient.connect();
    }

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
        String pat=System.getenv("PATH");
        pat = System.getenv("BOOTCLASSPATH");
        pat = System.getenv("ANDROID_ROOT");
        pat = System.getenv("ANDROID_DATA");
        pat = System.getenv("PATH");
        //connectWebSocket();
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
