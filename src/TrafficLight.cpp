#include <iostream>
#include <random>
#include "TrafficLight.h"
#include <chrono>
#include <future>

/* Implementation of class "MessageQueue" */
template<class T>
MessageQueue<T>::MessageQueue() {
    std::cout << "Construction of the MessageQueue Object." << std::endl;
}

template<class T>
MessageQueue<T>::~MessageQueue() {
    std::cout << "Destroying the MessageQueue Object." << std::endl;
}

template<typename T>
T MessageQueue<T>::receive() {
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait()
    // to wait for and receive new messages and pull them from the queue using move semantics.
    // The received object should then be returned by the receive function.
    std::unique_lock<std::mutex> unique_lock(_mutex_queue);
    _condition_variable.wait(unique_lock, [this] {
        return !_queue.empty();
    });
    T message = std::move(_queue.back());
    _queue.pop_back();
    return message;
}

template<typename T>
void MessageQueue<T>::send(T &&msg) {
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex>
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.
    std::lock_guard<std::mutex> lock_guard(_mutex_queue);
    _queue.push_back(std::move(msg));
    _condition_variable.notify_one();
}

/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight() {
    _currentPhase = TrafficLightPhase::red;
    _message_queue = std::make_shared<MessageQueue<TrafficLightPhase>>();
}

TrafficLight::~TrafficLight() {
    std::cout << "Destructor of traffic light" << std::endl;
}

void TrafficLight::waitForGreen() {
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop
    // runs and repeatedly calls the receive function on the message queue.
    // Once it receives TrafficLightPhase::green, the method returns.
    std::lock_guard<std::mutex> lockGuard(_mutex_traffic_light);
    while (true) {
        TrafficLightPhase traffic_phase = _message_queue->receive();
        if (traffic_phase == green) {
            break;
        }
    }
    _condition.notify_one();
}

TrafficLightPhase TrafficLight::getCurrentPhase() {
    return _currentPhase;
}

void TrafficLight::simulate() {
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called.
    // To do this, use the thread queue in the base class.
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

[[noreturn]] // virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases() {
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles
    // and toggles the current phase of the traffic light between red and green and sends an update method
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds.
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.
    std::random_device rd;
    std::mt19937 eng(rd());
    std::uniform_int_distribution<> distribution(4.0, 6.0);

    /* Print id of the current thread */

    auto last_update = std::chrono::system_clock::now();
    auto duration = distribution(eng);

    while (true) {

        //correct so far.
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - last_update);

        if (elapsed.count() >= duration) {
            switchTrafficLight();
            auto message = _currentPhase;
            auto ftr_wait_send = std::async(std::launch::async, &MessageQueue<TrafficLightPhase>::send, _message_queue,
                                            std::move(message));
            ftr_wait_send.wait();
            duration = distribution(eng);
            last_update = std::chrono::system_clock::now();
        }
    }
}

void TrafficLight::switchTrafficLight() {
    if (_currentPhase == red) {
        _currentPhase = green;
    } else {
        _currentPhase = red;
    }
}
