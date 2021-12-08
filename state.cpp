#include "engine.hpp"

#include <coroutine>

#include "state.hpp"

#include <boost/crc.hpp>

uint ObservableState::commandCount(const std::vector<uint>& which) {
    uint acm = 0;
    for (auto& idx : which) {
        // Don't worry size has constant time complexity as of c++11, it does not need to walk the list
        acm += instances[idx].commandList.size();
    }
    return acm;
}

#include <iostream>

CommandGenerator<CommandCoroutineType> ObservableState::getCommandGenerator(std::vector<uint> *which) {
    uint idx = 0;
    auto it = this->instances[(*which)[idx]].commandList.begin();
    while(true) {
        if (it != this->instances[(*which)[idx]].commandList.end()) {
            co_yield CommandCoroutineType(&(*it), it, idx, which);
            it++;
        } else {
            it = this->instances[(*which)[++idx]].commandList.begin();
        }
    }
}

void ObservableState::doUpdate(float timeDelta) {
    // This function is all wrong rn
    return;
    for (auto& inst : instances) {
        if (!inst.commandList.empty()) {
            const auto& cmd = inst.commandList.front();
            float len;
            glm::vec3 delta;
            switch (cmd.kind){
                case CommandKind::MOVE:
                    delta = cmd.data.dest - inst.position;
                    len = length(delta);
                    if (len > inst.entity->maxSpeed / 30.0f) {
                        inst.position += (inst.entity->maxSpeed * timeDelta / len) * delta;
                    } else {
                        inst.position = cmd.data.dest;
                        inst.commandList.pop_front();
                    }
                    break;
                case CommandKind::STOP:
                    inst.commandList.clear();
                    break;
            }
        }
    }
}

void ObservableState::syncToAuthoritativeState(AuthoritativeState& state) {
    uint highestSynced = 0;
    uint syncIndex = 0;
    state.lock.lock();
    for (int i = 0; i < instances.size(); i++) {
        if (instances[i].inPlay) {
            if (syncIndex >= state.instances.size() || state.instances[syncIndex].id > instances[i].id) {
                instances[i].orphaned = true;
            } else if (state.instances[syncIndex].id == instances[i].id) {
                instances[i].syncToAuthInstance(state.instances[syncIndex]);
                syncIndex++;
            } else {
                std::cerr << "Instance syncronization problem" << std::endl;
            }
        }
    }
    for (; syncIndex < state.instances.size(); syncIndex++) {
        instances.push_back(state.instances[syncIndex]);
    }
    state.lock.unlock();
    instances.erase(std::remove_if(instances.begin(), instances.end(), [](const auto& x){ return x.orphaned; }), instances.end());
}

#include "pathgen.hpp"

// TODO This function is really ineffecient and holds the auth state lock the whole time it runs
void AuthoritativeState::doUpdateTick() {
    if (frame % 30 == 1) std::cout << std::hex << crc() << std::endl;

    const float timeDelta = Net::secondsPerTick;
    std::scoped_lock l(lock);
    auto size = instances.size();
    for (int i = 0; i < size;) {
        bool removed = false;
        auto it = instances.begin() + i;
        assert(it->inPlay);
        if (it->entity->isProjectile) {
            for (auto& other : instances) {
                if (it->parentID == other.id || other.entity->isProjectile) continue;
                float d;
                float l = length(it->dP);
                if (other.rayIntersects(it->position, normalize(it->dP), d)) {
                    if (d < l * timeDelta) {
                        instances.erase(it);
                        removed = true;
                        // std::cout << it->id << " hit " << other.id << std::endl;
                    }
                }
                if (removed) break;
            }
            if (!removed) it->position += it->dP * timeDelta;
        } else if (!it->commandList.empty()) {
            // TODO check if unit is alive
            const auto& cmd = it->commandList.front();
            float distance;
            switch (cmd.kind){
                case CommandKind::MOVE:
                    distance = Pathgen::arrive(*it, cmd.data.dest);
                    if (it->commandList.size() > 1) {
                        if (distance < Pathgen::arrivalDeltaContinuing) {
                            it->commandList.pop_front();
                        }
                    } else {
                        if (distance < Pathgen::arrivalDeltaStopping) {
                            it->commandList.pop_front();
                        }
                    }
                    break;
                case CommandKind::STOP:
                    it->commandList.clear();
                    break;
                case CommandKind::TARGET_LOCATION:
                    it->target = Target(cmd.data.dest);
                    it->commandList.pop_front();
                    break;
                case CommandKind::TARGET_UNIT:
                    it->target = Target(cmd.data.id);
                    it->commandList.pop_front();
                    break;
            }
            for (auto& other : instances) {
                if (other == *it | other.entity->isProjectile) continue;
                Pathgen::collide(other, *it);
            }
        } else if (it->entity->isUnit) {
            Pathgen::stop(*it);
        }
        if (!removed) {
            for (const auto& ai : it->entity->ais) {
                ai->run(*it);
            }
            for (auto& weapon : it->weapons) {
                // TODO Weapon targeting
                weapon.fire(it->position);
            }
        }
        if (removed) size--;
        else i++;
    }
    frame++;
}

void AuthoritativeState::dump() {
    std::cout << "Have instances: { ";
    for (const auto& inst : this->instances) {
        std::cout << inst.id << " ";
    }
    std::cout << " }" << std::endl;
}

uint32_t AuthoritativeState::crc() {
    boost::crc_32_type crc;
    std::scoped_lock l(lock);
    crc.process_bytes(&frame, sizeof(frame));
    for (const auto& inst : instances) {
        inst.doCrc(crc);
    }
    return crc.checksum();
}

AuthoritativeState::AuthoritativeState(Base *context)
: context(context) { }

const static std::vector<CommandKind> fowardsable_ = {
    CommandKind::MOVE,
    CommandKind::STOP,
    CommandKind::TARGET_UNIT,
    CommandKind::TARGET_LOCATION
};

template<int N>
struct Forwardable {
    template<typename... TT>
    constexpr Forwardable(TT... tt)
    : data { tt... } {
        std::sort(data, data + N);
    }
    bool operator()(CommandKind kind) const {
        return std::binary_search(data, data + N, kind);
    }
private:
    CommandKind data[N];
};

static const Forwardable<4> forwardable(
    CommandKind::MOVE,
    CommandKind::STOP,
    CommandKind::TARGET_UNIT,
    CommandKind::TARGET_LOCATION
);

// constexpr auto fowardsable = [](){ std::sort(fowardsable_.begin(), fowardsable_.end()); return fowardsable_; }();

void AuthoritativeState::process(ApiProtocol *data, std::optional<std::shared_ptr<Networking::Session>> session) {
    
    // std::cout << *data << std::endl;

    std::vector<Instance>::iterator it;

    if (data->kind == ApiProtocolKind::COMMAND) {
        if (forwardable(data->command.kind)) {
            lock.lock();
            it = std::lower_bound(instances.begin(), instances.end(), data->command.id);
            if (it == instances.end() || *it != data->command.id) {
                lock.unlock();
                return;
            };
            switch (data->command.mode) {
                case InsertionMode::BACK:
                    it->commandList.push_back(data->command);
                    break;
                case InsertionMode::FRONT:
                    it->commandList.push_front(data->command);
                    break;
                case InsertionMode::OVERWRITE:
                    it->commandList.clear();
                    it->commandList.push_back(data->command);
                    break;
            }
            lock.unlock();
        } else if (data->command.kind == CommandKind::CREATE) {
            const auto ent = Api::context->currentScene->entities.at(data->buf);
            lock.lock();
            Instance inst;
            if (Api::context->headless) {
                inst = Instance(ent, counter++);
            } else {
                inst = Instance(ent, Api::context->currentScene->textures.data() + ent->textureIndex, Api::context->currentScene->models.data() + ent->modelIndex,
                    counter++, true);
            }
            inst.heading = data->command.data.heading;
            inst.position = data->command.data.dest;
            inst.team = data->command.data.id;
            context->authState.instances.push_back(std::move(inst));
            lock.unlock();
        } else if (data->command.kind == CommandKind::DESTROY) {
            lock.lock();
            it = std::lower_bound(instances.begin(), instances.end(), data->command.id);
            if (it == instances.end() || *it != data->command.id) {
                lock.unlock();
                return;
            };
            instances.erase(it);
            lock.unlock();
        }
    } else if (data->kind == ApiProtocolKind::FRAME_ADVANCE) {
        doUpdateTick();
        frame = data->frame;
    } else if (data->kind == ApiProtocolKind::PAUSE) {
        paused = (bool)data->frame;
    } else if (data->kind == ApiProtocolKind::TEAM_DECLARATION) {
        lock.lock();
        teams.push_back(Team((TeamID)data->frame, data->buf, session));
        lock.unlock();
    } else if (data->kind == ApiProtocolKind::RESOURCES) {
        lock.lock();
        auto it = find(teams.begin(), teams.end(), (TeamID)data->frame);
        if (it != teams.end()) it->resourceUnits = it->resourceUnits + data->dbl;
        lock.unlock();
    } else if (data->kind == ApiProtocolKind::SERVER_MESSAGE) {
        Api::eng_echo(data->buf);
    }
}

void AuthoritativeState::emit(const ApiProtocol& data) {
    context->send(data);
}

void AuthoritativeState::doCallbacks() {
    // std::cout << "pretend dispatching callbacks" << std::endl;
}