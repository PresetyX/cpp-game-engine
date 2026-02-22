#include "engine/physics/world.hpp"

#include "engine/math/transform.hpp"
#include "engine/physics/broadphase.hpp"
#include "engine/physics/components.hpp"
#include "engine/physics/integrate.hpp"
#include "engine/physics/solver.hpp"

namespace engine::physics {

namespace {

SolverBody solver_body(math::Transform& t, RigidBody* body) {
    if (body == nullptr) {
        return SolverBody{};  // static
    }
    return SolverBody{&t.position, &body->velocity, body->inverse_mass};
}

}  // namespace

void PhysicsWorld::step(Registry& registry, float dt) {
    contacts_.clear();

    store_previous_transforms(registry);
    integrate_bodies(registry, dt, settings_.gravity);

    std::vector<BroadphaseProxy> proxies;
    registry.view<const math::Transform, const Collider>().each(
        [&](Entity e, const math::Transform& t, const Collider& c) {
            proxies.push_back(
                BroadphaseProxy{e, collider_aabb(c, t.position), !registry.has<RigidBody>(e)});
        });

    BroadphaseOptions options;
    options.skip_static_pairs = true;
    const std::vector<BroadphasePair> pairs = sweep_and_prune(proxies, options);

    // Component pointers stay valid: nothing is added or removed during the step.
    std::vector<ContactConstraint> constraints;
    for (const BroadphasePair& pair : pairs) {
        const Collider& col_a = registry.get<Collider>(pair.a);
        const Collider& col_b = registry.get<Collider>(pair.b);
        math::Transform& ta = registry.get<math::Transform>(pair.a);
        math::Transform& tb = registry.get<math::Transform>(pair.b);
        RigidBody* rb_a = registry.try_get<RigidBody>(pair.a);
        RigidBody* rb_b = registry.try_get<RigidBody>(pair.b);

        // The trigger check comes before the "no dynamic body" discard, so a kinematic body
        // (or a static-vs-kinematic pair) is still detected inside a trigger.
        const bool trigger = col_a.is_trigger || col_b.is_trigger;
        const float inv_a = rb_a != nullptr ? rb_a->inverse_mass : 0.0f;
        const float inv_b = rb_b != nullptr ? rb_b->inverse_mass : 0.0f;
        if (!trigger && inv_a + inv_b <= 0.0f) {
            continue;
        }

        const auto contact = collide(col_a, ta.position, col_b, tb.position);
        if (!contact) {
            continue;
        }
        contacts_.push_back(ContactPair{pair.a, pair.b, *contact, trigger});
        if (trigger) {
            continue;
        }

        ContactConstraint c;
        c.a = solver_body(ta, rb_a);
        c.b = solver_body(tb, rb_b);
        c.normal = contact->normal;
        c.depth = contact->depth;
        c.friction = mix_friction(col_a.friction, col_b.friction);
        c.restitution = mix_restitution(col_a.restitution, col_b.restitution);
        constraints.push_back(c);
    }

    SolverSettings solver;
    solver.velocity_iterations = settings_.velocity_iterations;
    solver.penetration_slop = settings_.penetration_slop;
    solver.correction_percent = settings_.correction_percent;
    solver.restitution_threshold = settings_.restitution_threshold;
    solve_contacts(constraints, solver);
}

}  // namespace engine::physics
