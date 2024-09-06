#include "neuron/neuron.hpp"


#include <iostream>
#include <memory>

int main() {
    std::cout << "Running Neuron version: " << neuron::get_version() << std::endl;

    auto ctx = std::make_shared<neuron::Context>(neuron::ContextSettings{
        .application_name    = "neuron-example",
        .application_version = neuron::Version{0, 1, 0},
    });

}
