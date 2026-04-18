#pragma once

#include <memory>

#include "chronos/api/handlers/context.hpp"
#include "chronos/api/http/types.hpp"

namespace chronos::api::handlers {

http::HttpResponse HandleCreateVigilRemediationJob(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context);

http::HttpResponse HandleGetVigilRemediationJobById(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context);

http::HttpResponse HandleCancelVigilRemediationJob(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context);

http::HttpResponse HandleGetVigilActionStatus(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context);

}  // namespace chronos::api::handlers
