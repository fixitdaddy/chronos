#pragma once

#include <memory>

#include "chronos/api/handlers/context.hpp"
#include "chronos/api/http/types.hpp"
#include "chronos/persistence/in_memory/in_memory_repositories.hpp"

namespace chronos::api::handlers {

http::HttpResponse HandleGetDeadLetterExecutions(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context,
    const std::shared_ptr<persistence::in_memory::InMemoryExecutionRepository>& in_memory_execution_repo);

http::HttpResponse HandleQuarantineExecution(
    const http::HttpRequest& request,
    const std::shared_ptr<persistence::in_memory::InMemoryExecutionRepository>& in_memory_execution_repo);

}  // namespace chronos::api::handlers
