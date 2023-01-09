package com.example.bluetooth_kitkat;
import android.annotation.SuppressLint;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Set;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {

    // ==================================================== Partie bluetooth ==================================================== \\

    private static final int REQUEST_ENABLE_BT = 1;
    BluetoothAdapter mBluetoothAdapter;
    BluetoothSocket mmSocket;
    BluetoothDevice mmDevice;
    OutputStream mmOutputStream;
    InputStream mmInputStream;
    Thread workerThread;
    byte[] readBuffer;
    int readBufferPosition;
    volatile boolean stopWorker;

    // ==================================================== Partie graphe ==================================================== \\

    TextView receivedMessagesTextView;
    Button mgraph;
    String Data = "0/2/34/9/34/45/55/54/53/51/40/20/10/50+\n";
    String dataGlobe;

    @SuppressLint("MissingPermission")
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // ==================================================== Partie graphe ==================================================== \\

        mgraph = findViewById(R.id.graph);

        // ==================================================== Partie bluetooth ==================================================== \\

        Button sendButton = (Button) findViewById(R.id.send_button);
        receivedMessagesTextView = (TextView) findViewById(R.id.received_messages_text_view);

        // Vérifie si le Bluetooth est activé sur l'appareil
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBluetoothAdapter == null) {
            Toast.makeText(this, "Cet appareil ne prend pas en charge Bluetooth", Toast.LENGTH_SHORT).show();
            return;
        }
        if (!mBluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
        }
        // Trouve le module Bluetooth et ouvre une connexion
        Set<BluetoothDevice> pairedDevices = mBluetoothAdapter.getBondedDevices();
        if (pairedDevices.size() > 0) {
            for (BluetoothDevice device : pairedDevices) {
                if (device.getName().equals("HC-06")) { // Etape de connexion
                    mmDevice = device;
                    break;
                }
            }
        }
        UUID uuid = UUID.fromString("00001101-0000-1000-8000-00805F9B34FB"); // Standard SerialPortService ID du HC 06
        try {
            mmSocket = mmDevice.createRfcommSocketToServiceRecord(uuid);
            mmSocket.connect();
            mmOutputStream = mmSocket.getOutputStream();
            mmInputStream = mmSocket.getInputStream();
            beginListenForData();
            Toast.makeText(this, "Connecté au module Bluetooth", Toast.LENGTH_SHORT).show();
        } catch (IOException e) {
            e.printStackTrace();
        }

        // ==================================================== Boutons ==================================================== \\

        sendButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                String message = "1";
                sendData(message);
            }
        });

        mgraph.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                lancerGraph();
            }
        });
    }

    void lancerGraph(){
        Intent i = new Intent(MainActivity.this, Graph.class);
        i.putExtra("STRING", dataGlobe);
        startActivity(i);
    }

    void beginListenForData() {
        final Handler handler = new Handler();
        final byte delimiter = 10; // ASCII code for newline character

        stopWorker = false;
        readBufferPosition = 0;
        readBuffer = new byte[1024];
        workerThread = new Thread(new Runnable() {
            public void run() {
                while (!Thread.currentThread().isInterrupted() && !stopWorker) {
                    try {
                        int bytesAvailable = mmInputStream.available();
                        if (bytesAvailable > 0) {
                            byte[] packetBytes = new byte[bytesAvailable]; // tout les bytes qui sont available
                            mmInputStream.read(packetBytes);
                            for (int i = 0; i < bytesAvailable; i++) {
                                byte b = packetBytes[i];
                                Log.i("data", String.valueOf(b));
                                if (b == delimiter) {
                                    byte[] encodedBytes = new byte[readBufferPosition];
                                    System.arraycopy(readBuffer, 0, encodedBytes, 0, encodedBytes.length);
                                    final String data = new String(encodedBytes, "US-ASCII");
                                    Log.i("data", data);
                                    readBufferPosition = 0;
                                    i=bytesAvailable;
                                    handler.post(new Runnable() {
                                        public void run() {
                                            receivedMessagesTextView.append(data + "\n");
                                            dataGlobe = data;
                                        }
                                    });
                                } else {
                                    readBuffer[readBufferPosition++] = b;
                                }
                            }
                        }
                    } catch (IOException ex) {
                        stopWorker = true;
                    }
                }
            }
        });
        workerThread.start();
    }

    void sendData(String message) {
        try {
            mmOutputStream.write(message.getBytes());
            Toast.makeText(this, "Données envoyées au module Bluetooth", Toast.LENGTH_SHORT).show();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}


