package com.zengge.nbmanager.activities;

import android.annotation.SuppressLint;
import android.app.ProgressDialog;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.view.KeyEvent;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.LinearLayout;
import android.widget.Toast;

import androidx.appcompat.app.AppCompatActivity;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.zengge.nbmanager.R;
import com.zengge.nbmanager.adapter.ClassAdapter;
import com.zengge.nbmanager.adapter.ClassHolder;
import com.zengge.nbmanager.adapter.PackageAdapter;
import com.zengge.nbmanager.utils.DecompileFile;

import org.jetbrains.annotations.NotNull;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ThreadPoolExecutor;

import jadx.api.JadxArgs;
import jadx.api.JadxDecompiler;
import jadx.core.utils.exceptions.JadxException;
import com.zengge.nbmanager.jadx.gui.treemodel.JClass;
import com.zengge.nbmanager.jadx.gui.treemodel.JNode;
import com.zengge.nbmanager.jadx.gui.treemodel.JPackage;
import com.zengge.nbmanager.jadx.gui.treemodel.JSources;

public class PackageActivity extends AppCompatActivity {

    ProgressDialog progressDialog;
    private RecyclerView title_recycler_view;
    private RecyclerView recyclerView;
    private ClassAdapter classAdapter;
    private List<JNode> mJNodes = new ArrayList<>();
    private LinearLayout empty_rel;
    private PackageAdapter titleAdapter;
    private String tmpFile;
    private JadxDecompiler decompiler;
    public Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            progressDialog.cancel();
            open();
            Toast.makeText(PackageActivity.this, "Loading completed", Toast.LENGTH_LONG).show();
        }
    };

    private @NotNull ProgressDialog showWaitingDialog() {
        ProgressDialog waitingDialog =
                new ProgressDialog(PackageActivity.this);
        waitingDialog.setMessage(getString(R.string.jadx_loading));
        waitingDialog.setIndeterminate(true);
        waitingDialog.setCancelable(false);
        waitingDialog.show();
        return waitingDialog;
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        title_recycler_view = findViewById(R.id.title_recycler_view);
        title_recycler_view.setLayoutManager(new LinearLayoutManager(this, LinearLayoutManager.HORIZONTAL, false));
        titleAdapter = new PackageAdapter(this, new ArrayList<JNode>());
        title_recycler_view.setAdapter(titleAdapter);

        recyclerView = findViewById(R.id.recycler_view);

        classAdapter = new ClassAdapter(this, mJNodes);
        recyclerView.setLayoutManager(new LinearLayoutManager(this));
        recyclerView.setAdapter(classAdapter);

        empty_rel = findViewById(R.id.empty_rel);

        classAdapter.setOnItemClickListener((view, viewHolder, position) -> {
            if (viewHolder instanceof ClassHolder) {
                JNode jNode = mJNodes.get(position);
                if (R.drawable.ic_folder == jNode.getIcon()) {
                    JPackage jPackage = (JPackage) jNode;

                    getFile(jPackage);
                    refreshTitleState(jPackage);
                } else if (R.drawable.ic_folder != jNode.getIcon()) {
                    JClass jClass = (JClass) jNode;
                    ArrayList<String> info = new ArrayList<>();
                    info.add(jClass.getName() + ".java");
                    info.add(jClass.getCls().getCode());
                    DecompileFile.openDecodeIntent(PackageActivity.this, info);
                    Toast.makeText(PackageActivity.this, ((JClass) jNode).getFullName(), Toast.LENGTH_SHORT).show();
                }

            }
        });

        titleAdapter.setOnItemClickListener((view, viewHolder, position) -> {
            JNode jPackage = (JNode) titleAdapter.getItem(position);
            getFile(jPackage);

            int count = titleAdapter.getItemCount();
            int removeCount = count - position - 1;
            for (int i = 0; i < removeCount; i++) {
                titleAdapter.removeLast();
            }
        });
        progressDialog = showWaitingDialog();


        final String fileName = getIntent().getStringExtra("fileName");
        File inputFile = new File(fileName);
        setTitle(inputFile.getName());
        JadxArgs args = new JadxArgs();
        args.setSkipResources(true);
        args.setShowInconsistentCode(true);
        // TODO: JaDX 1.1.0
        //args.setInputFile(new File(fileName));
        decompiler = new JadxDecompiler(args);

        new Thread() {
            @Override
            public void run() {
                try {
                    // TODO: JaDX 1.1.0
                    //decompiler.load();
                    decompiler.loadFile(new File(fileName));
                } catch (JadxException e) {
                    // TODO: JaDX 1.1.0
                    //} catch (Exception e) {
                    Toast.makeText(PackageActivity.this, "Error message：" + e.toString(), Toast.LENGTH_SHORT).show();
                    finish();
                }
                Message msg = new Message();
                msg.what = 1;
                mHandler.sendMessage(msg);
            }
        }.start();

    }

    public boolean onCreateOptionsMenu(Menu menu) {
        getMenuInflater().inflate(R.menu.package_menu, menu);
        return super.onCreateOptionsMenu(menu);
    }

    @SuppressLint("NonConstantResourceId")
    public boolean onOptionsItemSelected(@NotNull MenuItem item) {
        switch (item.getItemId()) {
            case R.id.mass_decompile: {
                deCode(getIntent().getStringExtra("fileName"));
                break;
            }
            case R.id.jadx_settings: {
                startActivityForResult(new Intent(PackageActivity.this, JaDXSettingsActivity.class), 0);
                break;
            }
        }
        return super.onOptionsItemSelected(item);
    }

    public void deCode(@NotNull String fileName) {
        JadxArgs args = new JadxArgs();
        args.setOutDir(new File(fileName.substring(0, fileName.lastIndexOf(".")) + "_src"));
        // TODO: JaDX 1.1.0
        //args.setInputFile(new File(fileName));
        args.setThreadsCount(Runtime.getRuntime().availableProcessors());
        final JadxDecompiler decompiler = new JadxDecompiler(args);
        try {
            // TODO: JaDX 1.1.0
            //jadxDecompiler.load();
            decompiler.loadFile(new File(fileName));
        } catch (JadxException e) {
            // TODO: JaDX 1.1.0
            //} catch (Exception e) {
            Toast.makeText(this, "Error message：" + e.toString(), Toast.LENGTH_SHORT).show();
        }
        new JSources(decompiler);
        final ProgressDialog progressDialog = getProgressDialog(this, getString(R.string.jadx_saving_code));
        Runnable save = () -> {
            try {
                ThreadPoolExecutor ex = (ThreadPoolExecutor) decompiler.getSaveExecutor();
                ex.shutdown();
                while (ex.isTerminating()) {
                    long total = ex.getTaskCount();
                    long done = ex.getCompletedTaskCount();
                    progressDialog.setProgress((int) (done * 100.0 / (double) total));
                    Thread.sleep(300);
                }
                progressDialog.cancel();
                runOnUiThread(() -> Toast.makeText(PackageActivity.this, "Decompilation completed", Toast.LENGTH_SHORT).show());
            } catch (InterruptedException e) {
                PackageActivity packageActivity = PackageActivity.this;
                Toast.makeText(packageActivity, "Error message：" + e.toString(), Toast.LENGTH_SHORT).show();
            }
        };
        new Thread(save).start();
    }

    private @NotNull ProgressDialog getProgressDialog(Context context, String str) {
        ProgressDialog progressDialog = new ProgressDialog(context);
        progressDialog.setProgress(0);
        progressDialog.setTitle(str);
        progressDialog.setProgressStyle(1);
        progressDialog.setMax(100);
        progressDialog.show();
        return progressDialog;
    }

    public void open() {
        JSources jSources = new JSources(decompiler);
        refreshTitleState(jSources);
        getFile(jSources);
    }

    void refreshTitleState(JNode jPackage) {
        titleAdapter.addItem(jPackage);
    }

    public void getFile(JNode jNode) {
        new MyTask(jNode).executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR, "");
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK
                && event.getRepeatCount() == 0) {

            List<JNode> jNodes = (List<JNode>) titleAdapter.getAdapterData();
            if (jNodes.size() == 1) {
                finish();
            } else {
                titleAdapter.removeItem(jNodes.size() - 1);
                getFile(jNodes.get(jNodes.size() - 1));
            }
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    @SuppressLint("StaticFieldLeak")
    class MyTask extends AsyncTask {
        JNode sJNode;

        MyTask(JNode jNode) {
            sJNode = jNode;
        }

        @Override
        protected Object doInBackground(Object[] params) {
            List jNodes = new ArrayList<>();
            if (R.mipmap.packagefolder_obj == sJNode.getIcon()) {
                JSources jSources = (JSources) sJNode;
                jNodes = jSources.getRootPackage();
            } else if (R.drawable.ic_folder == sJNode.getIcon()) {
                if (sJNode != null) {
                    JPackage jPackage = (JPackage) sJNode;
                    jNodes.addAll(jPackage.getInnerPackages());
                    jNodes.addAll(jPackage.getClasses());
                }
            }
            mJNodes = jNodes;
            return jNodes;
        }

        @Override
        protected void onPostExecute(Object o) {
            if (mJNodes.size() > 0) {
                empty_rel.setVisibility(View.GONE);
            } else {
                empty_rel.setVisibility(View.VISIBLE);
            }
            classAdapter.refresh(mJNodes);
        }
    }
}