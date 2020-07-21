#include <faiss/index_factory.h>
#include <grpcpp/grpcpp.h>
#include <gtest/gtest.h>

#include "../src/index_service.h"
#include "index.grpc.pb.h"
#include "index.pb.h"

int DIM = 100;

static int port = 30000;

class IDMapFlatL2Test : public ::testing::Test {
 protected:
  IDMapFlatL2Test()
      : service_{faiss_serving::IndexService(
            *faiss::index_factory(DIM, "IDMap,Flat"))} {}

  void SetUp() override {
    server_address_ << "localhost:" << ++port;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address_.str(),
                             grpc::InsecureServerCredentials());
    builder.RegisterService(&service_);

    server_ = builder.BuildAndStart();
  }

  void TearDown() override { server_->Shutdown(); }

  void ResetStub() {
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(
        server_address_.str(), grpc::InsecureChannelCredentials());
    stub_ = faiss_serving::protos::index::Index::NewStub(channel);
  }

  std::unique_ptr<faiss_serving::protos::index::Index::Stub> stub_;
  std::unique_ptr<grpc::Server> server_;
  std::ostringstream server_address_;
  faiss_serving::IndexService service_;
};

void ExpectNtotal(
    std::unique_ptr<faiss_serving::protos::index::Index::Stub> &stub,
    const int ntotal) {
  grpc::ClientContext ctx;
  faiss_serving::protos::index::GetNtotalRequest request;
  faiss_serving::protos::index::GetNtotalReply reply;

  auto status = stub->GetNtotal(&ctx, request, &reply);

  EXPECT_TRUE(status.ok());
  EXPECT_EQ(reply.n(), ntotal);
}

TEST_F(IDMapFlatL2Test, TestAll) {
  ResetStub();

  srand48(123L);

  const int N = 10000;
  const int ID_OFFSET = 333;

  {
    grpc::ClientContext ctx;
    faiss_serving::protos::index::AddWithIdRequest request;
    faiss_serving::protos::index::AddWithIdReply reply;

    for (int i = 0; i < N; i++) {
      request.add_ids(i * ID_OFFSET);

      for (int j = 0; j < DIM; j++) {
        request.add_vectors(float(drand48()));
      }
    }

    auto status = stub_->AddWithId(&ctx, request, &reply);

    EXPECT_TRUE(status.ok());
  }

  ExpectNtotal(stub_, N);

  {
    grpc::ClientContext ctx;
    faiss_serving::protos::index::SearchRequest request;
    faiss_serving::protos::index::SearchReply reply;

    for (int i = 0; i < 10; i++) {
      for (int j = 0; j < DIM; j++) {
        request.add_vectors(float(drand48()));
      }
    }

    request.set_k(5);

    auto status = stub_->Search(&ctx, request, &reply);

    EXPECT_TRUE(status.ok());

    // TODO: Check content
  }

  const int REMOVE_N = 100;

  {
    grpc::ClientContext ctx;
    faiss_serving::protos::index::RemoveIdsRequest request;
    faiss_serving::protos::index::RemoveIdsReply reply;

    for (int i = 0; i < REMOVE_N; i++) {
      request.add_ids(i * ID_OFFSET);
    }

    auto status = stub_->RemoveIds(&ctx, request, &reply);

    EXPECT_TRUE(status.ok());
  }

  ExpectNtotal(stub_, N - REMOVE_N);

  {
    grpc::ClientContext ctx;
    faiss_serving::protos::index::ResetRequest request;
    faiss_serving::protos::index::ResetReply reply;

    auto status = stub_->Reset(&ctx, request, &reply);

    EXPECT_TRUE(status.ok());
  }

  ExpectNtotal(stub_, 0);
}
