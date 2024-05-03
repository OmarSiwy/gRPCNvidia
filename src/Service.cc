#include "Service.h"
#include "AuthMiddleware.h"
#include "GeminiCon.h"

ServerImpl::ServerImpl() {
  // On Service Start
}

ServerImpl::~ServerImpl() {
  // On Service Stop
  this->server_->Shutdown();
  this->cq_->Shutdown();
}

void ServerImpl::Run(uint16_t port) {
  std::string server_address = "0.0.0.0:" + std::to_string(port);
  ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service_);
  
  // Firebase Auth Interceptor
  std::vector<std::unique_ptr<ServerInterceptorFactoryInterface>> creators;
  creators.push_back(std::unique_ptr<ServerInterceptorFactoryInterface>(new LoggingInterceptorFactory()));
  builder.experimental().SetInterceptorCreators(std::move(creators));

  this->cq_ = builder.AddCompletionQueue();
  this->server_ = builder.BuildAndStart();

  // Check if both are initialized correctly
  if (!this->cq_ || !this->server_) {
    std::cerr << "Initialization failed for server components." << std::endl;
    return; // Exit or handle the error appropriately
  }

  std::cout << "Server listening on " << server_address << std::endl;
  HandleRpcs();
}

void ServerImpl::HandleRpcs() {
  // Create Methods that the function needs
  std::cout << "reached setup";
  new CallData(&service_, cq_.get(), CallData::UPLOAD_IMAGE);
  new CallData(&service_, cq_.get(), CallData::PING);

  void *tag;
  bool ok;
  std::cout << "reached functions";
  while (true) {
    cq_->Next(&tag, &ok);
    assert(ok);
    static_cast<CallData *>(tag)->Proceed();
  }
}

CallData::CallData(RouteGuide::AsyncService *service, ServerCompletionQueue *cq,
                   Type type)
    : type_(type), status_(CREATE), service_(service), cq_(cq), stream_(&ctx_),
       responder_(&ctx_) {
  Proceed();
}

void CallData::Proceed() {
  switch (status_) {
  case CREATE: {
    std::cout << "request asked";
    if (type_ == UPLOAD_IMAGE)
      service_->RequestUploadImage(&ctx_, &stream_, cq_, cq_, this);
    else if (type_ == PING)
      service_->RequestPing(&ctx_, &ping_request, &responder_, cq_, cq_, this);
    status_ = PROCESS;
    std::cout << "request passed";
    break;
  }
  case PROCESS: {
    auto start = std::chrono::high_resolution_clock::now();

    new CallData(service_, cq_, type_);

    std::cout << "reached functions";
    if (type_ == UPLOAD_IMAGE)
      UploadImage();
    else if (type_ == PING)
      Ping();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    std::cout << "SimpleRPC duration: " << elapsed.count() << " ms\n";

    break;
  }
  case FINISH: {
    delete this;
    break;
  }
  default: {
    break;
  }
  }
}

inline void CallData::UploadImage() {
  std::cout << "Uploading Image?";
  std::vector<uint8_t> imageData;

  // Reading all chunks sent by the client
  do {
    stream_.Read(&chunk, this);
    std::vector<uint8_t> data(chunk.data().begin(), chunk.data().end());
    imageData.insert(imageData.end(), data.begin(), data.end());
  } while (chunk.data().size() > 0);

  // Once all chunks have been read, send a response back
  UploadStatus status;

  // Process Image using Gemini
  const std::pair<std::string, bool> returnmessage = GeminiCon::SendRequests(imageData);
  status.set_message(returnmessage.first);
  status.set_success(returnmessage.second);

  status_ = FINISH;

  // Sending the response back to the client
  stream_.Finish(status, grpc::Status::OK, this);
}

inline void CallData::Ping() {
  // Log or handle the ping request
  std::cout << "Received Ping: " << ping_request.data() << std::endl;
  // Respond back to client
  ping_response.set_data("Pong: " + ping_request.data());
  status_ = FINISH;
  // Sending the response back to the client
  responder_.Finish(ping_response, grpc::Status::OK, this);
}

/* Examples
grpc::Status ServiceImpl::SimpleRPC(grpc::ServerContext* context, const
routeguide::Point* point, routeguide::Feature* feature) override { auto start =
std::chrono::high_resolution_clock::now();

  // Processing
  feature->set_name("SimpleRPC");
  // End of Processing

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> elapsed = end - start;
  std::cout << "SimpleRPC duration: " << elapsed.count() << " ms\n";

  return grpc::Status::OK;
}

grpc::Status ServiceImpl::ServerStreaming(grpc::ServerContext* context, const
grpc::ServerReader<routeguide::Point>* reader, routeguide::RouteSummary*
summary) override { auto start = std::chrono::high_resolution_clock::now();

  routeguide::Point point;
  while (reader->Read(&point)) {
    // Simulate some processing
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> elapsed = end - start;
  std::cout << "ServerStreaming duration: " << elapsed.count() << " ms\n";

  return grpc::Status::OK;
}

grpc::Status ServiceImpl::ClientStreaming(grpc::ServerContext* context,
grpc::ServerReaderWriter<routeguide::Feature, routeguide::Point>* stream)
override { auto start = std::chrono::high_resolution_clock::now();

  routeguide::Point point;
  while (stream->Read(&point)) {
    // Simulate some processing
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> elapsed = end - start;
  std::cout << "ClientStreaming duration: " << elapsed.count() << " ms\n";

  return grpc::Status::OK;
}

grpc::Status ServerImpl::BidirectionalStream(grpc::ServerContext* context,
grpc::ServerReaderWriter<routeguide::Feature, routeguide::Point>* stream)
override { auto start = std::chrono::high_resolution_clock::now();

  routeguide::Point point;
  while (stream->Read(&point)) {
    // Simulate some processing
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double, std::milli> elapsed = end - start;
  std::cout << "BidirectionalStream duration: " << elapsed.count() << " ms\n";

  return grpc::Status::OK;
}
*/
