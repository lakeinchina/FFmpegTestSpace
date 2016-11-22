package me.lake.decoder_mp42yuv;

import android.os.Bundle;
import android.support.v7.app.AppCompatActivity;
import android.util.Log;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;

import me.lake.ffmpeg.FFmpeg;
import me.lake.ffmpeg.NativeMediaData;

public class MainActivity extends AppCompatActivity {
    SurfaceView sv_main;
    boolean isDestroy;
    TextView tv_info;
    float ratio;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        isDestroy = false;
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        tv_info = (TextView) findViewById(R.id.tv_info);
        sv_main = (SurfaceView) findViewById(R.id.sv_main);
        findViewById(R.id.btn_start).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread() {
                    @Override
                    public void run() {
                        final NativeMediaData mediaData = FFmpeg.open("/sdcard/ffmpeg.mp4");
                        ratio = mediaData.height / (float) mediaData.width;
                        sv_main.post(new Runnable() {
                            @Override
                            public void run() {
                                ViewGroup.LayoutParams layoutParams = sv_main.getLayoutParams();
                                if (ratio <= 1.0) {
                                    layoutParams.height = (int) (sv_main.getWidth() * ratio);
                                    layoutParams.width = sv_main.getWidth();
                                } else {
                                    layoutParams.height = (int) (sv_main.getWidth() / 2 * ratio);
                                    layoutParams.width = sv_main.getWidth() / 2;
                                }
                                sv_main.setLayoutParams(layoutParams);
                                tv_info.setText(mediaData.toString());
                            }
                        });
                        Log.e("aa", "NativeMediaData=" + mediaData);
                        long interval = (long) (1000.0 / mediaData.frameRate);
                        long lasttime = 0;
                        while (!isDestroy && 0 == FFmpeg.decodeNextFrame(mediaData.nativePointer, sv_main.getHolder().getSurface())) {
                            try {
                                long t = interval - System.currentTimeMillis() + lasttime;
                                lasttime = System.currentTimeMillis();
                                if (t > 0) {
                                    sleep(t);
                                }
                            } catch (InterruptedException e) {
                                e.printStackTrace();
                            }
                        }
                        FFmpeg.close(mediaData.nativePointer);
                    }
                }.start();
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        isDestroy = true;
    }
}
