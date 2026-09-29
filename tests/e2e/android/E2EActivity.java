package org.psyche.dobby;

import android.app.Activity;
import android.os.Bundle;
import android.widget.TextView;

public final class E2EActivity extends Activity {
    static { System.loadLibrary("dobby_e2e_jni"); }
    private static native int runTests();

    @Override public void onCreate(Bundle state) {
        super.onCreate(state);
        TextView result = new TextView(this);
        setContentView(result);
        result.setText(runTests() == 0 ? "Dobby E2E passed" : "Dobby E2E failed");
    }
}
