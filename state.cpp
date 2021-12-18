#include "engine.hpp"

#include <coroutine>

#include "state.hpp"

#include <boost/crc.hpp>

uint ObservableState::commandCount(const std::vector<uint>& which) {
    uint acm = 0;
    for (auto& idx : which) {
        // Don't worry size has constant time complexity as of c++11, it does not need to walk the list
        acm += instances[idx]->commandList.size();
    }
    return acm;
}

#include <iostream>

CommandGenerator<CommandCoroutineType> ObservableState::getCommandGenerator(std::vector<uint> *which) {
    uint idx = 0;
    auto it = this->instances[(*which)[idx]]->commandList.begin();
    while(true) {
        if (it != this->instances[(*which)[idx]]->commandList.end()) {
            co_yield CommandCoroutineType(&(*it), it, idx, which);
            it++;
        } else {
            it = this->instances[(*which)[++idx]]->commandList.begin();
        }
    }
}

void ObservableState::doUpdate(float timeDelta) {
    // This function is all wrong rn
    return;
    for (auto& inst : instances) {
        if (!inst->commandList.empty()) {
            const auto& cmd = inst->commandList.front();
            float len;
            glm::vec3 delta;
            switch (cmd.kind){
                case CommandKind::MOVE:
                    delta = cmd.data.dest - inst->position;
                    len = length(delta);
                    if (len > inst->entity->maxSpeed / 30.0f) {
                        inst->position += (inst->entity->maxSpeed * timeDelta / len) * delta;
                    } else {
                        inst->position = cmd.data.dest;
                        inst->commandList.pop_front();
                    }
                    break;
                case CommandKind::STOP:
                    inst->commandList.clear();
                    break;
            }
        }
    }
}

void ObservableState::syncToAuthoritativeState(AuthoritativeState& state) {
    uint highestSynced = 0;
    uint syncIndex = 0;
    // TODO!!! Waiting on this lock is bad (This is the biggest rendering performance thing to fix)
    state.lock.lock();
    bool idSelectionChange = false;
    for (int i = 0; i < instances.size(); i++) {
        if (instances[i]->inPlay) {
            if (syncIndex >= state.instances.size() || state.instances[syncIndex]->id > instances[i]->id) {
                instances[i]->orphaned = true;
                static_cast<Engine *>(Api::context)->apiLock.lock();
                auto it = find_if(static_cast<Engine *>(Api::context)->idsSelected.begin(),
                    static_cast<Engine *>(Api::context)->idsSelected.end(), [&](auto id) { return id == instances[i]->id; });
                if (it != static_cast<Engine *>(Api::context)->idsSelected.end()) {
                    idSelectionChange = true;
                    static_cast<Engine *>(Api::context)->idsSelected.erase(it);
                }
                static_cast<Engine *>(Api::context)->apiLock.unlock();
            } else if (state.instances[syncIndex]->id == instances[i]->id) {
                instances[i]->syncToAuthInstance(*state.instances[syncIndex]);
                syncIndex++;
            } else {
                instances[i]->orphaned = true;
                static_cast<Engine *>(Api::context)->apiLock.lock();
                auto it = find_if(static_cast<Engine *>(Api::context)->idsSelected.begin(),
                    static_cast<Engine *>(Api::context)->idsSelected.end(), [&](auto id) { return id == instances[i]->id; });
                if (it != static_cast<Engine *>(Api::context)->idsSelected.end()) {
                    idSelectionChange = true;
                    static_cast<Engine *>(Api::context)->idsSelected.erase(it);
                }
                static_cast<Engine *>(Api::context)->apiLock.unlock();
            }
        }
    }
    for (; syncIndex < state.instances.size(); syncIndex++) {
        instances.push_back(new Instance(*state.instances[syncIndex]));
    }
    state.lock.unlock();
    instances.erase(std::remove_if(instances.begin(), instances.end(), [](const auto& x){
        if (x->orphaned) {
            delete x;
            return true;
        }
        return false;
    }), instances.end());
    if (idSelectionChange) {
        GuiCommandData *what = new GuiCommandData();
        what->str = "onSelectionChanged";
        static_cast<Engine *>(Api::context)->gui->submitCommand({ Gui::GUI_NOTIFY, what });
    }
    beams = state.beams;
}

ObservableState::~ObservableState() {
    for (auto inst : instances) delete inst;
}

#include "pathgen.hpp"

#define BOOST_STACKTRACE_USE_ADDR2LINE
#include <boost/stacktrace.hpp>

// TODO This function is ineffecient and holds the auth state lock the whole time it runs (maybe making a copy and working on that would be better)
// Also shoving all the instances in a vector is simple, but really we probably want a spacial partitioning system
void AuthoritativeState::doUpdateTick() {
    if (frame.load(std::memory_order_relaxed) % 300 == 1) std::cout << std::hex << crc() << std::endl;

    std::array<std::vector<std::pair<Instance *, float>>, ::Config::maxTeams + 1> buildPowerAllocations;
    const float timeDelta = Config::Net::secondsPerTick;
    std::scoped_lock l(lock);
    auto copy = instances;
    instances.clear();
    beams.clear();
    std::vector<Instance *> toDelete;
    for (auto& it : copy) {
        if (!it) continue;
        if (it->health < 0) {
            toDelete.push_back(it);
            it = nullptr;
            continue;
        }
        assert(it->inPlay);
        if (it->entity->isProjectile) {
            for (auto& other : copy) {
                if (!other || it->parentID == other->id || other->entity->isProjectile || !other->hasCollision) continue;
                float d;
                float l = length(it->dP);
                if (other->rayIntersects(it->position, normalize(it->dP), d)) {
                    if (d < l * timeDelta) {
                        other->health = other->health - it->entity->weapon->damage;
                        toDelete.push_back(it);
                        it = nullptr;
                        break;
                    }
                }
            }
            if (it) {
                it->position += it->dP * timeDelta;
                if (++it->framesAlive > it->entity->framesTillDead) {
                    toDelete.push_back(it);
                    it = nullptr;
                }
            }
        } else if (it && !it->commandList.empty() && !it->uncompleted) {
            const auto& cmd = it->commandList.front();
            float distance;
            switch (cmd.kind){
                case CommandKind::MOVE:
                    distance = it->secondQueuedCommandRequiresMovement() ? Pathgen::seek(*it, cmd.data.dest) : Pathgen::arrive(*it, cmd.data.dest);
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
            if (!it->isBuilding && it->entity->buildPower > 0) {
                auto bit = find_if(it->commandList.begin(), it->commandList.end(), [](const auto& x) -> bool { return x.kind == CommandKind::BUILD; });
                if (bit != it->commandList.end()) {
                    const auto ent = Api::context->currentScene->entities.at(bit->data.buf);
                    Instance *inst;
                    if (Api::context->headless) {
                        inst = new Instance(ent, counter++);
                    } else {
                        inst = new Instance(ent, Api::context->currentScene->textures.data() + ent->textureIndex,
                            Api::context->currentScene->models.data() + ent->modelIndex, counter++, true);
                    }
                    inst->heading = it->heading;
                    inst->position = it->position;
                    inst->dP = it->dP;
                    inst->parentID = it->id;
                    inst->uncompleted = true;
                    inst->team = it->team;
                    inst->hasCollision = false;
                    it->commandList.erase(bit);
                    it->isBuilding = true;
                    inst->buildPower = it->entity->buildPower;
                    instances.push_back(inst);
                }
            }
        } else if (it && it->entity->isUnit && !it->uncompleted) {
            Pathgen::stop(*it);
        }
        if (it) {
            if (it->buildPower) {
                buildPowerAllocations[it->team].push_back({ it, it->buildPower });
            }
            if (it->hasCollision) for (auto other : copy) {
                if (!other || *other == *it || other->entity->isProjectile || !other->hasCollision) continue;
                Pathgen::collide(*other, *it);
            }
            for (const auto& ai : it->entity->ais) {
                ai->run(*it);
            }
            float closest = std::numeric_limits<float>::max();
            for (auto& weapon : it->weapons) {
                glm::vec3 vec = { 0.0f, 0.0f, 0.0f };
                bool fireBeam = false;
                for (auto other : copy) {
                    if (!other || other->entity->isProjectile || !other->team || other->team == it->team) continue;
                    auto dist = distance(other->position, it->position);
                    if (weapon.kindOf == WeaponKind::PLASMA_CANNON && dist < weapon.instanceOf->range * 1.5f && dist < closest) {
                        vec = Pathgen::computeBalisticTrajectory(it->position, other->position, other->dP,
                            weapon.instanceOf->range, weapon.instanceOf->entity->v_m);
                        closest = dist;
                    } else if (weapon.kindOf == WeaponKind::BEAM && dist < weapon.instanceOf->range && dist < closest) {
                        fireBeam = true;
                        vec = other->position;
                        closest = dist;
                    }
                }
                if (fireBeam || distance2(vec, { 0.0f, 0.0f, 0.0f }) > 0.01) weapon.fire(it->position, vec);
            }
        }
    }
    for (int i = 0; i < beams.size(); i++) {
        auto length = distance(beams[i].b, beams[i].a);
        auto normed = normalize(beams[i].b - beams[i].a);
        for (auto it : copy) {
            if (!it || it->entity->isProjectile || beamDatum[i].parent == it->id) continue;
            float dist;
            bool hit = it->rayIntersects(beams[i].a, normed, dist);
            if (!hit || dist > length) continue;
            beams[i].b = beams[i].a + normed * dist;
            if (it->entity->isUnit) {
                it->health -= beamDatum[i].damage;
            }
            break;
        }
    }
    for (const auto& buildPowerAllocation : buildPowerAllocations) {
        if (buildPowerAllocation.empty()) continue;
        float totalWantedThisTick = 0.0f;
        for (const auto [inst, bp] : buildPowerAllocation) {
            totalWantedThisTick += bp;
        }
        totalWantedThisTick *= Config::Net::secondsPerTick;
        float fraction = 1.0f;
        if (totalWantedThisTick > teams[buildPowerAllocation.front().first->team]->resourceUnits) {
            fraction = totalWantedThisTick / teams[buildPowerAllocation.front().first->team]->resourceUnits;
        }
        teams[buildPowerAllocation.front().first->team]->resourceUnits -= totalWantedThisTick * fraction;
        for (auto [inst, bp] : buildPowerAllocation) {
            inst->resources += bp * Config::Net::secondsPerTick * fraction;
            if (inst->resources >= inst->entity->resources) {
                // refund the overage
                teams[buildPowerAllocation.front().first->team]->resourceUnits += inst->resources - inst->entity->resources;
                inst->resources = inst->entity->resources;
                inst->hasCollision = true;
                inst->uncompleted = false;
                inst->buildPower = 0.0f;
                auto oit = find_if(copy.begin(), copy.end(), [inst](auto x){ return x && x->id == inst->parentID; });
                if (oit != copy.end()) {
                    (*oit)->isBuilding = false;
                    // Idk what is wrong with this, but something is very wrong, it seems to break like 3 different things 
                    // and causes segfaults on shutdown (commands are trivially copyable, so I don't know what could possibly be wrong)
                    // if (inst->commandList.empty()) {
                    //     copy_if((*oit)->commandList.begin(), (*oit)->commandList.end(), inst->commandList.end(),
                    //         [](const auto& x) -> bool { return x.kind != CommandKind::BUILD; });
                    // }
                }
            }
        }
    }
    frame.fetch_add(1, std::memory_order_relaxed);
    copy.erase(std::remove(copy.begin(), copy.end(), nullptr), copy.end());
    for (auto inst : toDelete) delete inst;
    copy.reserve(copy.size() + instances.size());
    copy.insert(copy.end(), instances.begin(), instances.end());
    instances = std::move(copy);
}

void AuthoritativeState::dump() const {
    std::cout << "Have instances: {" << std::endl;
    for (const auto& inst : this->instances) {
        std::cout << inst->id << "(" << inst->entity->name << ") - " << inst->position << std::endl;
    }
    std::cout << "}" << std::endl;
}

uint32_t AuthoritativeState::crc() const {
    boost::crc_32_type crc;
    std::scoped_lock l(lock);
    auto tmp = frame.load(std::memory_order_relaxed);
    crc.process_bytes(&tmp, sizeof(tmp));
    for (const auto& inst : instances) {
        inst->doCrc(crc);
    }
    return crc.checksum();
}

AuthoritativeState::AuthoritativeState(Base *context)
: context(context) {}

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

static const Forwardable<5> forwardable(
    CommandKind::MOVE,
    CommandKind::STOP,
    CommandKind::TARGET_UNIT,
    CommandKind::TARGET_LOCATION,
    CommandKind::BUILD
);

#include "api_util.hpp"

void AuthoritativeState::process(ApiProtocol *data, std::optional<std::shared_ptr<Networking::Session>> session) {
    
    if (data->kind == ApiProtocolKind::COMMAND) {
        if (forwardable(data->command.kind)) {
            lock.lock();
            auto itx = getInstance(data->command.id);
            if (itx == instances.end() || **itx != data->command.id) {
                lock.unlock();
                return;
            };
            switch (data->command.mode) {
                case InsertionMode::BACK:
                    (*itx)->commandList.push_back(data->command);
                    break;
                case InsertionMode::FRONT:
                    (*itx)->commandList.push_front(data->command);
                    break;
                case InsertionMode::OVERWRITE:
                    (*itx)->commandList.clear();
                    (*itx)->commandList.push_back(data->command);
                    break;
            }
            lock.unlock();
        } else if (data->command.kind == CommandKind::CREATE) {
            const auto ent = Api::context->currentScene->entities.at(data->buf);
            lock.lock();
            Instance *inst;
            if (Api::context->headless) {
                inst = new Instance(ent, counter++);
            } else {
                inst = new Instance(ent, Api::context->currentScene->textures.data() + ent->textureIndex,
                    Api::context->currentScene->models.data() + ent->modelIndex, counter++, true);
            }
            inst->heading = data->command.data.heading;
            inst->position = data->command.data.dest;
            inst->team = data->command.data.id;
            if (data->callbackID && session) {
                ApiProtocol data2 = { ApiProtocolKind::CALLBACK };
                data2.callbackID = data->callbackID;
                data2.frame = frame.load(std::memory_order_relaxed);
                data2.command.id = inst->id;
                callbacks.push({ data2, session.value() });
            }
            instances.push_back(inst);
            lock.unlock();
        } else if (data->command.kind == CommandKind::DESTROY) {
            lock.lock();
            auto itx = getInstance(data->command.id);
            if (itx == instances.end() || **itx != data->command.id) {
                lock.unlock();
                return;
            };
            delete *itx;
            instances.erase(itx);
            lock.unlock();
        }
    } else if (data->kind == ApiProtocolKind::FRAME_ADVANCE) {
        assert(!context->headless);
        doUpdateTick();
        frame.store(data->frame, std::memory_order_relaxed);
    } else if (data->kind == ApiProtocolKind::PAUSE) {
        paused = (bool)data->frame;
    } else if (data->kind == ApiProtocolKind::TEAM_DECLARATION) {
        lock.lock();
        teams[data->frame] = std::make_shared<Team>((TeamID)data->frame, data->buf, session);
        if (session.has_value()) {
            session.value()->team = teams[data->frame];
        }
        lock.unlock();
    } else if (data->kind == ApiProtocolKind::RESOURCES) {
        if (data->frame > Config::maxTeams) {
            std::cerr << "Team id exceeds max teams " << std::dec << data->frame << std::endl;
            return;
        }
        lock.lock();
        auto tmp = teams[data->frame];
        if (!tmp) {
            std::cerr << "Invalid team " << std::dec << data->frame << std::endl;
            lock.unlock();
            return;
        }
        tmp->resourceUnits += data->dbl;
        lock.unlock();
    } else if (data->kind == ApiProtocolKind::SERVER_MESSAGE) {
        if (!context->headless) Api::eng_echo(data->buf);
    } else if (data->kind == ApiProtocolKind::CALLBACK) {
        assert(!context->headless);
        ApiUtil::doCallbackDispatch(data->callbackID, *data);
    }
}

void AuthoritativeState::emit(const ApiProtocol& data) const {
    context->send(data);
}

void AuthoritativeState::doCallbacks() {
    while (!callbacks.empty()) {
        callbacks.front().second->writeData(callbacks.front().first);
        callbacks.pop();
    }
}

void AuthoritativeState::enableCallbacks() {
    Api::context->lua->enableCallbacksOnThisThread();
}

AuthoritativeState::~AuthoritativeState() {
    for (auto inst : instances) delete inst;
}