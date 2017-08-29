/*
 *  Copyright (c) 2016, https://github.com/zhatalk
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

#ifndef NEBULA_NET_RPC_ZRPC_SERVER_HANDLER_H_
#define NEBULA_NET_RPC_ZRPC_SERVER_HANDLER_H_

#include <wangle/channel/Handler.h>

#include "nebula/net/zproto/zproto_package_data.h"
#include "nebula/net/handler/nebula_base_handler.h"

using ZRpcServerPipeline = wangle::Pipeline<folly::IOBufQueue&, ProtoRpcResponsePtr>;

// A real rpc server would probably use generated client/server stubs
class ZRpcServerHandler : public nebula::NebulaBaseHandler, public wangle::Handler<
  PackageMessagePtr, RpcRequestPtr,
  ProtoRpcResponsePtr, std::unique_ptr<folly::IOBuf>> {
public:
    explicit ZRpcServerHandler(nebula::ServiceBase* service)
      : NebulaBaseHandler(service) {}

    virtual void read(Context* ctx, PackageMessagePtr msg) override;
  
    virtual folly::Future<folly::Unit> write(Context* ctx, ProtoRpcResponsePtr rsp) override;
    
    virtual void readEOF(Context* ctx) override;
    virtual void readException(Context* ctx, folly::exception_wrapper e) override;
    
    virtual void transportActive(Context* ctx) override;
    virtual void transportInactive(Context* ctx) override;

    virtual folly::Future<folly::Unit> close(Context* ctx) override;
};

#endif
