package me.lake.encoder_yuv2h264;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.widget.Button;


public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {
    Button btn_encode;
    SurfaceView sv_preview;
    RESVideoClient videoClient;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        sv_preview = (SurfaceView) findViewById(R.id.sv_preview);
        sv_preview.getHolder().addCallback(this);
        btn_encode = (Button) findViewById(R.id.btn_encode);
        btn_encode.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                if ("start".equals(btn_encode.getText())) {
                    btn_encode.setText("stop");
                    videoClient.start();
                } else {
                    btn_encode.setText("start");
                    videoClient.stop();
                }
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    public void surfaceCreated(SurfaceHolder surfaceHolder) {
        videoClient = new RESVideoClient(surfaceHolder);
        videoClient.prepare();
    }

    @Override
    public void surfaceChanged(SurfaceHolder surfaceHolder, int i, int i1, int i2) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder surfaceHolder) {
        videoClient.destroy();
    }
}
