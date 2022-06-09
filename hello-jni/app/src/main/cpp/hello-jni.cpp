/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <jni.h>
#include "CBConfig.h"

#include "DatabaseManager.h"
#include "ReplicatorManager.h"

#include <thread>

std::shared_ptr<DatabaseManager> databaseManager;
std::shared_ptr<ReplicatorManager> replicatorManager;

CBLProxySettings proxySettings = {kCBLProxyHTTP, FLString(), 0, FLString(), FLString()};


void initManagers(std::string databasePath)
{
    /*
    CBLLog_SetConsoleLevel(kCBLLogInfo);

    CBLLog_SetCallbackLevel(kCBLLogInfo);

    // Couchbase logs that are too long to be reported.
    CBLLog_SetCallback([](CBLLogDomain domain,
                          CBLLogLevel  level,
                          FLString     message) {
        if (level >= kCBLLogInfo) {
            std::string messageStr = std::string(message);
            thAccountsLogDebug("[Couchbase] ", messageStr.c_str());
        }
    });
     */
    thAccountsLogDebug("[HELLO] ===", databasePath.c_str());
    databaseManager = std::make_shared<DatabaseManager>();
    databaseManager->setDatabasePath(databasePath);
    databaseManager->setServerHostname("concepts-test.tophatch.com");

    databaseManager->loadDatabaseCreateIfNeeded();

    replicatorManager = std::make_shared<ReplicatorManager>(&proxySettings);

    replicatorManager->setServerHostname("concepts-test.tophatch.com");


    if (!databaseManager->getDatabase())
    {
        thAccountsLogDebug("[HELLO] NULL DATABASE", databasePath.c_str());
    }
    replicatorManager->initReplicatorConfigurator(databaseManager->getDatabase());

    replicatorManager->sync();
}

void onResume()
{
    CBLReplicatorActivityLevel replicatorActivity = replicatorManager->replicatorStatus();

    if (replicatorActivity == CBLReplicatorActivityLevel::kCBLReplicatorOffline || replicatorActivity == CBLReplicatorActivityLevel::kCBLReplicatorStopped)
    {
        replicatorManager->stop();
        replicatorManager->sync();
    }
}

void onStop()
{
    replicatorManager->stop();
}

/* This is a trivial JNI example where we use a native method
 * to return a new VM String. See the corresponding Java source
 * file located at:
 *
 *   hello-jni/app/src/main/java/com/example/hellojni/HelloJni.java
 */
extern "C"
{

JNIEXPORT void JNICALL
Java_com_example_hellojni_HelloJni_stopReplication(JNIEnv *env, jobject thiz) {
    onStop();
}

JNIEXPORT void JNICALL
Java_com_example_hellojni_HelloJni_resumeReplication(JNIEnv *env, jobject thiz) {
    onResume();
}
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_hellojni_HelloJni_init(JNIEnv *env, jobject jObj, jstring database_path) {
    jsize       utf16Length = env->GetStringLength(database_path);
    jsize       utf8Length  = env->GetStringUTFLength(database_path);
    std::string result((size_t)utf8Length, ' ');
    env->GetStringUTFRegion(database_path, 0, utf16Length, result.data());

    thAccountsLogDebug("[HELLO]", result.c_str());
    initManagers(result);
}