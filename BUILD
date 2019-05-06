load("//tensorflow:tensorflow.bzl", "tf_cc_binary")

tf_cc_binary(
    name = "play",
    srcs = ["engine.hpp", "player.hpp", "engine.cpp", "play.cpp", "player.cpp"],
    copts = [ "-fexceptions", "-Ofast", "-Wall", "-Wno-unused-variable",
        "-Wno-unused-value", "-Wno-comment", "-Wno-unused-but-set-variable",
        "-Wno-maybe-uninitialized", "-Wdelete-non-virtual-dtor"
    ],
    deps = [
        "//tensorflow/cc:cc_ops",
        "//tensorflow/cc:client_session",
        "//tensorflow/core:tensorflow",
    ],
)
