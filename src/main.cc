#include <faiss/index_factory.h>
#include <faiss/index_io.h>
#include <gflags/gflags.h>
#include <grpcpp/grpcpp.h>

#include "index_service.h"

DEFINE_string(host, "0.0.0.0", "server host");
DEFINE_string(port, "8080", "server port");
DEFINE_int64(dimension, 100, "faiss index dimension");
DEFINE_string(index_factory, "IVF100,Flat", "faiss index factory");
DEFINE_string(index_file_path, "", "path to faiss index file");

int main(int argc, char **argv) {
  gflags::SetUsageMessage("A gRPC server for Faiss");
  gflags::SetVersionString("0.1.0");
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  std::string server_address(FLAGS_host + ":" + FLAGS_port);

  faiss::Index *faiss_index =
      FLAGS_index_file_path.empty()
          ? faiss::index_factory(FLAGS_dimension, FLAGS_index_factory.c_str())
          : faiss::read_index(FLAGS_index_file_path.c_str());

  faiss_serving::IndexService index_service(*faiss_index);

  grpc::ServerBuilder builder;

  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  builder.RegisterService(&index_service);

  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  server->Wait();

  return 0;
}
