//
// Copyright (C) 2019 Adofo Martinez <adolfo at ipcaster dot net>
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#pragma once

namespace ipcaster
{

/**
 * Base class for subject objects implementing the Observer pattern
 */
template <class TObserver>
class Subject
{
public:

    /**
     * Add a new observer for this object
     * 
     * @params observer Weak reference to the observer object
     */
    void attachObserver(std::weak_ptr<TObserver> observer)
    {
        observers_.push_back({observer, nullptr});
    }

    /**
     * Add a new observer for this object
     * A strong reference to the observer object is holded so it
     * will not be destroyed, at least, while not detached from this
     * object
     * 
     * @params observer Strong reference to the observer object
     */
    void attachObserverStrong(std::shared_ptr<TObserver> observer)
    {
        observers_.push_back({observer, observer});
    }

    /**
     * Remove an observer for this object
     * 
     * @params observer_to_detach Reference to the observer to be detached
     */
    void detachObserver(TObserver& observer_to_detach)
    {
        for (auto it = observers_.cbegin(); it != observers_.cend(); ) {
            if (auto observer = it.observer_weak->lock()) {
                if (observer.get() == &observer_to_detach) {
                    it = observers_.erase(it);
                    break; 
                }
                it++;
            }
            else 
                it = observers_.erase(it);
        }
    }

protected:

    /** Holds always a weak reference and sometimes also a strong reference 
     * to the observer */
    struct Observer
    {
        std::weak_ptr<TObserver> observer_weak;
        std::shared_ptr<TObserver> observer_strong;
    };

    /** Registered observers for this object */
    std::vector<Observer> observers_;
};

}