package me.lake.decoder_mp42yuv;

import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.support.v7.app.AppCompatActivity;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.SeekBar;
import android.widget.TextView;

import me.lake.ffmpeg.FFmpeg;
import me.lake.ffmpeg.NativeMediaData;

@SuppressWarnings("all")
public class MainActivity extends AppCompatActivity {
    SurfaceView sv_main;
    boolean isDestroy;
    TextView tv_info;
    SeekBar sb_progress;
    float ratio;

    final Object syncIsSeeking = new Object();
    boolean isSeeking = false;

    private Handler workHandler;
    private HandlerThread workHandlerThread;
    int seekPos = 0;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        isDestroy = false;
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        tv_info = (TextView) findViewById(R.id.tv_info);
        sv_main = (SurfaceView) findViewById(R.id.sv_main);
        sb_progress = (SeekBar) findViewById(R.id.sb_progress);
        findViewById(R.id.btn_start).setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                workHandler.sendEmptyMessage(MSG_PLAY);
            }
        });
        sb_progress.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {
                if (!b) {
                    return;
                }
                synchronized (syncIsSeeking) {
                    int dst = (int) (mediaData.duration / 1000000 * i / 100);
                    if (seekPos != dst) {
                        seekPos = dst;
                        workHandler.sendMessage(workHandler.obtainMessage(MSG_SEEK, seekPos, 0));
                    }
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                synchronized (syncIsSeeking) {
                    workHandler.removeMessages(MSG_DECODE);
                    isSeeking = true;
                }
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                synchronized (syncIsSeeking) {
                    workHandler.sendEmptyMessage(MSG_DECODE);
                    isSeeking = false;
                }
            }
        });
        workHandlerThread = new HandlerThread("RESRtmpSender,workHandlerThread");
        workHandlerThread.start();
        workHandler = new Handler(workHandlerThread.getLooper()) {
            @Override
            public void handleMessage(Message msg) {
                switch (msg.what) {
                    case MSG_PLAY: {
                        mediaData = FFmpeg.open("/sdcard/ffmpegtime.mp4");
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
                        interval = (long) (1000.0 / mediaData.frameRate);
                        lasttime = 0;

                        long t = interval - System.currentTimeMillis() + lasttime;
                        lasttime = System.currentTimeMillis();
                        sendEmptyMessage(MSG_DECODE);
                        break;
                    }
                    case MSG_DECODE: {
                        final long ret = FFmpeg.decodeNextFrame(mediaData.nativePointer, sv_main.getHolder().getSurface());
                        if (ret >= 0) {
                            long t = interval - System.currentTimeMillis() + lasttime;
                            lasttime = System.currentTimeMillis();
                            synchronized (syncIsSeeking) {
                                if (!isSeeking) {
                                    sendEmptyMessageDelayed(MSG_DECODE, t);
                                }
                            }
                            sb_progress.post(new Runnable() {
                                @Override
                                public void run() {
                                    synchronized (syncIsSeeking) {
                                        if (!isSeeking) {
                                            sb_progress.setProgress((int) (100 * ret / (mediaData.duration / 1000000)));
                                        }
                                    }
                                }
                            });
                        }
                        break;
                    }
                    case MSG_SEEK: {
                        FFmpeg.seekTo(mediaData.nativePointer, msg.arg1+1);
                        FFmpeg.decodeNextFrame(mediaData.nativePointer, sv_main.getHolder().getSurface());
                        break;
                    }
                    case MSG_STOP: {
                        removeCallbacksAndMessages(null);
                        FFmpeg.close(mediaData.nativePointer);
                        break;
                    }
                }
            }
        };
    }

    private static final int MSG_PLAY = 1;
    private static final int MSG_DECODE = 2;
    private static final int MSG_SEEK = 3;
    private static final int MSG_STOP = 4;
    NativeMediaData mediaData;
    long interval;
    long lasttime;

    @Override
    protected void onDestroy() {
        super.onDestroy();
        workHandler.sendEmptyMessage(MSG_STOP);
    }
}
