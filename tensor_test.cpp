#include "tensorflow/core/public/session.h"
#include "tensorflow/core/protobuf/meta_graph.pb.h"
#include <utility>

using namespace std;
using namespace tensorflow;

// module load java
// module load tensorflow/1.5_gpu
// module load bazel


// to build run
// first build take ~10 minutes, only about 10 seconds after the first time
// bazel build :test --action_env="LD_LIBRARY_PATH=${LD_LIBRARY_PATH}"
// output file at ../../bazel-bin/tensorflow/othello/test


int main()
{
    printf("running tensor_test\n");

    // set up your input paths
    const string pathToGraph = "data/model_0/model.ckpt.meta";
    const string checkpointPath = "data/model_0/model.ckpt";

    auto session = NewSession(SessionOptions());
    if (session == nullptr) 
    {
        throw runtime_error("Could not create Tensorflow session.");
    }

    Status status;

    // Read in the protobuf graph we exported
    MetaGraphDef graph_def;
    status = ReadBinaryProto(Env::Default(), pathToGraph, &graph_def);
    if (!status.ok()) 
    {
        throw runtime_error("Error reading graph definition from " + pathToGraph + ": " + status.ToString());
    }

    // Add the graph to the session
    status = session->Create(graph_def.graph_def());
    if (!status.ok())
    {
        throw runtime_error("Error creating graph: " + status.ToString());
    }

    // Read weights from the saved checkpoint
    Tensor checkpointPathTensor(DT_STRING, TensorShape());
    checkpointPathTensor.scalar<std::string>()() = checkpointPath;
    status = session->Run(
            {{ graph_def.saver_def().filename_tensor_name(), checkpointPathTensor },},
            {},
            {graph_def.saver_def().restore_op_name()},
            nullptr);
    if (!status.ok()) 
    {
        throw runtime_error("Error loading checkpoint from " + checkpointPath + ": " + status.ToString());
    }

    // from https://stackoverflow.com/questions/35508866/tensorflow-different-ways-to-export-and-run-graph-in-c/43639305#43639305
    // auto feedDict = ...
    // auto outputOps = ...
    // std::vector<tensorflow::Tensor> outputTensors;
    // status = session->Run(feedDict, outputOps, {}, &outputTensors);

    // from https://github.com/JackyTung/tensorgraph/blob/master/loadgraph/mnist.cc
    // preparing input to net
    
    int dim = 128;
    int* feed_ints = (int*) malloc(128 * sizeof(int));
    for(int i = 0; i < dim; i++)
    {
        feed_ints[i] = i+1;
    }

    tensorflow::Tensor x(tensorflow::DT_FLOAT, tensorflow::TensorShape({1, dim}));
    auto flattened_x = x.flat<float>().data();
    std::copy_n(feed_ints, dim, flattened_x);

    std::vector<std::pair<std::string, tensorflow::Tensor>> inputs = {
    { "x", x}
    };

    std::vector<tensorflow::Tensor> outputTensors;
    std::string output_tensor_name = "policy_head_output";
    printf("running batch\n");
    status = session->Run(inputs, {output_tensor_name}, {}, &outputTensors);
    printf("finished running batch\n");
    free(feed_ints);
    return 0;
}