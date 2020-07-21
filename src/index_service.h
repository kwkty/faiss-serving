#ifndef FAISS_GRPC_SERVER_SRC_INDEX_SERVICE_H_
#define FAISS_GRPC_SERVER_SRC_INDEX_SERVICE_H_

#include <faiss/Index.h>

#include <iostream>
#include <memory>
#include <string>

#include "index.grpc.pb.h"

namespace faiss_serving {
class IndexService final
    : public faiss_serving::protos::index::Index::Service {
 public:
  explicit IndexService(faiss::Index &index);

  grpc::Status Search(
      grpc::ServerContext *context,
      const faiss_serving::protos::index::SearchRequest *request,
      faiss_serving::protos::index::SearchReply *reply) override;

  grpc::Status Add(
      grpc::ServerContext *context,
      const faiss_serving::protos::index::AddRequest *request,
      faiss_serving::protos::index::AddReply *reply) override;

  grpc::Status AddWithId(
      grpc::ServerContext *context,
      const faiss_serving::protos::index::AddWithIdRequest *request,
      faiss_serving::protos::index::AddWithIdReply *reply) override;

  grpc::Status RemoveIds(
      grpc::ServerContext *context,
      const faiss_serving::protos::index::RemoveIdsRequest *request,
      faiss_serving::protos::index::RemoveIdsReply *reply) override;

  grpc::Status GetNtotal(
      grpc::ServerContext *context,
      const faiss_serving::protos::index::GetNtotalRequest *request,
      faiss_serving::protos::index::GetNtotalReply *reply) override;

  grpc::Status Train(
      grpc::ServerContext *context,
      const faiss_serving::protos::index::TrainRequest *request,
      faiss_serving::protos::index::TrainReply *reply) override;

  grpc::Status IsTrained(
      grpc::ServerContext *context,
      const faiss_serving::protos::index::IsTrainedRequest *request,
      faiss_serving::protos::index::IsTrainedReply *reply) override;

  grpc::Status Reset(
      grpc::ServerContext *context,
      const faiss_serving::protos::index::ResetRequest *request,
      faiss_serving::protos::index::ResetReply *reply) override;

 private:
  faiss::Index &index_;
};
}  // namespace faiss_serving

#endif  // FAISS_GRPC_SERVER_SRC_INDEX_SERVICE_H_
