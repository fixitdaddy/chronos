#include "chronos/api/handlers/dead_letter_handlers.hpp"

#include <cstddef>
#include <string>

namespace chronos::api::handlers {

http::HttpResponse HandleGetDeadLetterExecutions(
    const http::HttpRequest& request,
    const std::shared_ptr<HandlerContext>& context,
    const std::shared_ptr<persistence::in_memory::InMemoryExecutionRepository>& in_memory_execution_repo) {
  (void)request;
  context->metrics->requests_total.fetch_add(1);

  const auto items = in_memory_execution_repo->GetDeadLetterExecutions(100, 0);
  std::string body = "{\"items\":[";
  for (std::size_t i = 0; i < items.size(); ++i) {
    const auto& e = items[i];
    body += "{\"execution_id\":\"" + e.execution_id +
            "\",\"attempt_count\":" + std::to_string(e.attempt_count) +
            ",\"last_error_code\":\"" + e.last_error_code.value_or("") +
            "\"}";
    if (i + 1 < items.size()) {
      body += ",";
    }
  }
  body += "]}";

  return {.status = 200,
          .body = body,
          .headers = {{"content-type", "application/json"}}};
}

http::HttpResponse HandleQuarantineExecution(
    const http::HttpRequest& request,
    const std::shared_ptr<persistence::in_memory::InMemoryExecutionRepository>& in_memory_execution_repo) {
  const auto it = request.path_params.find("execution_id");
  if (it == request.path_params.end()) {
    return {.status = 400,
            .body = R"({"error":{"code":"VALIDATION_ERROR","message":"execution_id path param missing"}})",
            .headers = {{"content-type", "application/json"}}};
  }

  const auto ok = in_memory_execution_repo->MarkExecutionQuarantined(it->second);
  if (!ok) {
    return {.status = 404,
            .body = R"({"error":{"code":"NOT_FOUND","message":"execution not found"}})",
            .headers = {{"content-type", "application/json"}}};
  }

  return {.status = 200,
          .body = "{\"execution_id\":\"" + it->second + "\",\"quarantined\":true}",
          .headers = {{"content-type", "application/json"}}};
}

}  // namespace chronos::api::handlers
