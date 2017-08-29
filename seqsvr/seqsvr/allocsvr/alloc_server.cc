/*
 *  Copyright (c) 2016, https://github.com/nebula-im
 *  All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// TODO(@benqi): 使用zrpc-code-gen代码生成工具自动生成

#include "allocsvr/alloc_server.h"
#include "nebula/net/net_engine_manager.h"

#include "allocsvr/allocsvr_manager.h"

bool AllocServer::Initialize() {
  RegisterService("alloc_server", "rpc_server", "zrpc");
  RegisterService("store_client", "rpc_client", "zrpc");

  BaseServer::Initialize();

#if 0
  // one
  timer_manager_->ScheduleOneShotTimeout([]() {
    LOG(INFO) << "ScheduleOneShotTimeout!!!!";
  }, 1000);

  // once
  timer_manager_->ScheduleRepeatingTimeout([]() {
    static int i = 0;
    LOG(INFO) << "ScheduleRepeatingTimeout - " << i++;
  }, 1000);
#endif

  return true;
}

bool AllocServer::Run() {
  auto net_engine_manager = nebula::NetEngineManager::GetInstance();
  // 启动成功
  try {
    net_engine_manager->Start();
  } catch (std::exception& e) {
    LOG(ERROR) << "Run - catch exception: " << e.what();
    return -1;
  } catch (...) {
    LOG(ERROR) << "Run - catch  a invalid exception";
    return -1;
  }

  auto alloc_manager = AllocSvrManager::GetInstance();

  main_eb_.runAfterDelay([&] {
    auto alloc_service = net_engine_manager->Lookup("alloc_server");
    auto alloc_name = std::string("alloc_server:10000");  // +
    //
    folly::to<std::string>(alloc_service->GetServiceConfig().port);
    // auto alloc_name =
    //     std::string("alloc_server") +
    //     folly::to<std::string>(alloc_service->GetServiceConfig().port);
    // LOG(INFO) << "alloc_service->GetServiceConfig().port :"
    //           << alloc_service->GetServiceConfig().port;
    // GPerftoolsProfiler profiler;
    // profiler.ProfilerStart();
    alloc_manager->Initialize(timer_manager_.get(), "set_cluster_1",
                              alloc_name);

    // auto store_instance = StoreSvrManager::GetInstance();
    // store_instance->Initialize(1, "/tmp/seq.dat");
  }, 1000);

  BaseDaemon::Run();

  // profiler.ProfilerStop();

  net_engine_manager->Stop();
  alloc_manager->Destroy();

  return true;
}

int main(int argc, char* argv[]) {
  return nebula::DoMain<AllocServer>(argc, argv);
}
