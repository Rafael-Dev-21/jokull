package com.rafael04th.jokull;

import android.app.Activity;
import android.os.Bundle;

public class JokullActivity extends Activity {

    private GameViewWrapper wrapper;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        wrapper = new GameViewWrapper(this);
        setContentView(wrapper);
    }

    @Override
    protected void onDestroy() {
      super.onDestroy();
      wrapper.release();
    }
}
