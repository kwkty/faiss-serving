#include "index_service.h"

#include <faiss/impl/AuxIndexStructures.h>

namespace faiss_serving {

IndexService::IndexService(faiss::Index &index) : index_{index} {}

grpc::Status IndexService::Search(
    grpc::ServerContext *context,
    const faiss_serving::protos::index::SearchRequest *request,
    faiss_serving::protos::index::SearchReply *reply) {
  const auto k = request->k();

  const auto &vectors = request->vectors();

  if (vectors.size() % index_.d != 0) {
    return grpc::Status(
        grpc::StatusCode::INVALID_ARGUMENT,
        "query field must be divisible by the dimension of index");
  }

  const auto n = vectors.size() / index_.d;

  std::vector<float> distances(n * k);
  std::vector<faiss::Index::idx_t> ids(n * k);

  index_.search(n, vectors.data(), k, distances.data(), ids.data());

  for (int i = 0; i < k; i++) {
    for (int j = 0; j < n; j++) {
      reply->add_results()->add_distances(distances[i * index_.d * j]);
      reply->add_results()->add_ids(ids[i * index_.d * j]);
    }
  }

  return grpc::Status::OK;
}

grpc::Status IndexService::Add(
    grpc::ServerContext *context,
    const faiss_serving::protos::index::AddRequest *request,
    faiss_serving::protos::index::AddReply *reply) {
  const auto &vectors = request->vectors();

  if (vectors.size() % index_.d != 0) {
    return grpc::Status(
        grpc::StatusCode::INVALID_ARGUMENT,
        "vectors field must be divisible by the dimension of index");
  }

  try {
    index_.add(vectors.size() / index_.d, vectors.data());
  } catch (std::exception &err) {
    return grpc::Status(grpc::StatusCode::ABORTED, err.what());
  }

  return grpc::Status::OK;
}

grpc::Status IndexService::AddWithId(
    grpc::ServerContext *context,
    const faiss_serving::protos::index::AddWithIdRequest *request,
    faiss_serving::protos::index::AddWithIdReply *reply) {
  const auto &vectors = request->vectors();

  if (vectors.size() % index_.d != 0) {
    return grpc::Status(
        grpc::StatusCode::INVALID_ARGUMENT,
        "vectors field must be divisible by the dimension of index");
  }

  const auto &ids = request->ids();
  const auto n = ids.size();

  if (vectors.size() != n * index_.d) {
    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                        "the dimension of vectors field must be equal to the "
                        "dimension of ids field");
  }

  try {
    index_.add_with_ids(n, vectors.data(), ids.data());
  } catch (std::exception &err) {
    return grpc::Status(grpc::StatusCode::ABORTED, err.what());
  }

  return grpc::Status::OK;
}

grpc::Status IndexService::GetNtotal(
    grpc::ServerContext *context,
    const faiss_serving::protos::index::GetNtotalRequest *request,
    faiss_serving::protos::index::GetNtotalReply *reply) {
  reply->set_n(index_.ntotal);

  return grpc::Status::OK;
}

grpc::Status IndexService::RemoveIds(
    grpc::ServerContext *context,
    const faiss_serving::protos::index::RemoveIdsRequest *request,
    faiss_serving::protos::index::RemoveIdsReply *reply) {
  const auto &ids = request->ids();

  auto selector = faiss::IDSelectorBatch(ids.size(), ids.data());

  index_.remove_ids(selector);

  return grpc::Status::OK;
}

grpc::Status IndexService::Train(
    grpc::ServerContext *context,
    const faiss_serving::protos::index::TrainRequest *request,
    faiss_serving::protos::index::TrainReply *reply) {
  const auto &vectors = request->vectors();

  const int n = vectors.size() / index_.d;

  try {
    index_.train(n, vectors.data());
  } catch (std::exception &err) {
    return grpc::Status(grpc::StatusCode::ABORTED, err.what());
  }

  return grpc::Status::OK;
}

grpc::Status IndexService::IsTrained(
    grpc::ServerContext *context,
    const faiss_serving::protos::index::IsTrainedRequest *request,
    faiss_serving::protos::index::IsTrainedReply *reply) {
  reply->set_is_trained(index_.is_trained);

  return grpc::Status::OK;
}

grpc::Status IndexService::Reset(
    grpc::ServerContext *context,
    const faiss_serving::protos::index::ResetRequest *request,
    faiss_serving::protos::index::ResetReply *reply) {
  index_.reset();

  return grpc::Status::OK;
}

}  // namespace faiss_serving
