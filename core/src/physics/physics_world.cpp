#include "physics/physics_world.hpp"

#define CLASS_NAME "PhysicsWorld"
#include "log_macros.hpp"

#include "components/box_collider.hpp"
#include "components/plane_collider.hpp"
#include "components/rigidbody.hpp"
#include "components/sphere_collider.hpp"
#include "components/transform.hpp"
#include "math/math.hpp"
#include "scene/world_object.hpp"
#include "scene/world_object_manager.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

namespace {

// =============================================================================
// Representações em espaço de mundo dos colisores.
//
// A caixa agora é uma OBB (oriented bounding box): além do centro e das
// meias-extensões, carrega os 3 eixos ortonormais vindos da orientação
// (quatérnion) do Transform. É isso que faz o collider girar junto com a malha e
// permite tratar o cubo tombando de forma correta (via SAT), fechando a lacuna
// que o AABB deixava.
// =============================================================================

struct OBB {
    WorldObject* object = nullptr;
    Rigidbody* body = nullptr; ///< nullptr = estático (massa/inércia infinitas).
    Vector3 center{};          ///< Centro em mundo.
    Vector3 axis[3]{};         ///< Eixos locais X,Y,Z em mundo (ortonormais).
    Vector3 half{};            ///< Meias-extensões por eixo local.
};

struct WorldSphere {
    WorldObject* object = nullptr;
    Rigidbody* body = nullptr;
    Vector3 center{};
    float radius = 0.0f;
};

/// @brief Um ponto de contato: posição em mundo, normal (de A para B) e penetração.
struct Contact {
    WorldObject* objA = nullptr;
    Rigidbody* bodyA = nullptr;
    WorldObject* objB = nullptr;
    Rigidbody* bodyB = nullptr;
    Vector3 point{};
    Vector3 normal{}; ///< Unitária, aponta de A para B.
    float penetration = 0.0f;
    float restitution = 0.0f;
    float friction = 0.5f;
};

// -----------------------------------------------------------------------------
// Construção dos colisores em mundo.
// -----------------------------------------------------------------------------

Vector3 colliderCenter(WorldObject* obj, const Vector3& localCenter) {
    return obj->getTransform().getPosition() +
           Yume::Math::rotateVector(obj->getTransform().getRotationQuat(), localCenter);
}

OBB makeOBB(WorldObject* obj, BoxCollider* bc) {
    OBB o;
    o.object = obj;
    o.body = obj->getComponent<Rigidbody>();
    const Quaternion q = obj->getTransform().getRotationQuat();
    o.center = colliderCenter(obj, bc->getCenter());
    o.axis[0] = Yume::Math::rotateVector(q, {1.0f, 0.0f, 0.0f});
    o.axis[1] = Yume::Math::rotateVector(q, {0.0f, 1.0f, 0.0f});
    o.axis[2] = Yume::Math::rotateVector(q, {0.0f, 0.0f, 1.0f});
    o.half = bc->getHalfExtents();
    return o;
}

/// @brief Os 8 vértices da OBB em mundo.
void obbVertices(const OBB& o, Vector3 out[8]) {
    int k = 0;
    for (int sx = -1; sx <= 1; sx += 2)
        for (int sy = -1; sy <= 1; sy += 2)
            for (int sz = -1; sz <= 1; sz += 2) {
                out[k++] = o.center + o.axis[0] * (o.half.x * sx) + o.axis[1] * (o.half.y * sy) +
                           o.axis[2] * (o.half.z * sz);
            }
}

// -----------------------------------------------------------------------------
// Física do corpo: inércia, velocidade de ponto, impulsos.
// -----------------------------------------------------------------------------

Vector3 bodyCenterOfMass(WorldObject* obj) {
    if (auto* sc = obj->getComponent<SphereCollider>())
        return colliderCenter(obj, sc->getCenter());
    if (auto* bc = obj->getComponent<BoxCollider>())
        return colliderCenter(obj, bc->getCenter());
    return obj->getTransform().getPosition();
}

void setupInertia(WorldObject* obj, Rigidbody* body) {
    const float m = body->getMass();
    if (auto* sc = obj->getComponent<SphereCollider>()) {
        float r = sc->getRadius();
        float I = 0.4f * m * r * r;
        float invI = (I > 1e-6f) ? 1.0f / I : 0.0f;
        body->setInverseInertia({invI, invI, invI});
        return;
    }
    if (auto* bc = obj->getComponent<BoxCollider>()) {
        Vector3 he = bc->getHalfExtents();
        float lx = 2.0f * he.x, ly = 2.0f * he.y, lz = 2.0f * he.z;
        float k = m / 12.0f;
        float Ix = k * (ly * ly + lz * lz);
        float Iy = k * (lx * lx + lz * lz);
        float Iz = k * (lx * lx + ly * ly);
        body->setInverseInertia({Ix > 1e-6f ? 1.0f / Ix : 0.0f, Iy > 1e-6f ? 1.0f / Iy : 0.0f,
                                 Iz > 1e-6f ? 1.0f / Iz : 0.0f});
        return;
    }
    body->setInverseInertia({0.0f, 0.0f, 0.0f});
}

/**
 * @brief Aplica o inverso do tensor de inércia em ESPAÇO DE MUNDO a um vetor.
 *
 * O tensor é diagonal no espaço do corpo; em mundo vale I⁻¹_world = R·I⁻¹·Rᵀ.
 * Então: leva v para o espaço do corpo (Rᵀ·v = rotateVector(conj(q), v)),
 * multiplica pela diagonal, e volta ao mundo (R·... = rotateVector(q, ...)).
 * Para a esfera (isotrópica) isso é equivalente à diagonal direta, mas para a
 * caixa é o que faz o giro responder corretamente à orientação atual.
 */
Vector3 applyWorldInvInertia(WorldObject* obj, const Vector3& invIdiag, const Vector3& v) {
    const Quaternion q = obj->getTransform().getRotationQuat();
    Vector3 local = Yume::Math::rotateVector(Yume::Math::conjugate(q), v);
    local = {invIdiag.x * local.x, invIdiag.y * local.y, invIdiag.z * local.z};
    return Yume::Math::rotateVector(q, local);
}

Vector3 pointVelocity(Rigidbody* body, const Vector3& r) {
    if (!body)
        return {0.0f, 0.0f, 0.0f};
    return body->getVelocity() + Yume::Math::cross(body->getAngularVelocity(), r);
}

/// @brief Resolve a resposta de velocidade (normal + atrito) de UM contato.
void resolveContactVelocity(const Contact& c) {
    WorldObject* objA = c.objA;
    WorldObject* objB = c.objB;
    Rigidbody* bodyA = c.bodyA;
    Rigidbody* bodyB = c.bodyB;
    const Vector3& normal = c.normal;

    const Vector3 rA = bodyA ? (c.point - bodyCenterOfMass(objA)) : Vector3{0, 0, 0};
    const Vector3 rB = bodyB ? (c.point - bodyCenterOfMass(objB)) : Vector3{0, 0, 0};

    const float invMassA = bodyA ? bodyA->getInverseMass() : 0.0f;
    const float invMassB = bodyB ? bodyB->getInverseMass() : 0.0f;
    const Vector3 invIA = bodyA ? bodyA->getInverseInertia() : Vector3{0, 0, 0};
    const Vector3 invIB = bodyB ? bodyB->getInverseInertia() : Vector3{0, 0, 0};

    Vector3 relVel = pointVelocity(bodyB, rB) - pointVelocity(bodyA, rA);
    float velAlongNormal = Yume::Math::dot(relVel, normal);
    if (velAlongNormal > 0.0f)
        return;

    // Termo angular do denominador para a direção `dir`, com inércia em mundo:
    //   dir · ( (I⁻¹_world (r×dir)) × r )
    auto angularTerm = [&](WorldObject* obj, const Vector3& r, const Vector3& invI,
                           const Vector3& dir) -> float {
        if (!obj)
            return 0.0f;
        Vector3 rxd = Yume::Math::cross(r, dir);
        Vector3 iRxd = applyWorldInvInertia(obj, invI, rxd);
        Vector3 term = Yume::Math::cross(iRxd, r);
        return Yume::Math::dot(term, dir);
    };

    float denomN = invMassA + invMassB + angularTerm(objA, rA, invIA, normal) +
                   angularTerm(objB, rB, invIB, normal);
    if (denomN <= 1e-8f)
        return;

    float jn = -(1.0f + c.restitution) * velAlongNormal / denomN;
    Vector3 impulseN = normal * jn;
    float jnForFriction = -velAlongNormal / denomN;

    auto applyImpulse = [](WorldObject* obj, Rigidbody* body, float sign, const Vector3& imp,
                           const Vector3& r, const Vector3& invI) {
        if (!body)
            return;
        body->setVelocity(body->getVelocity() + imp * (sign * body->getInverseMass()));
        Vector3 torque = Yume::Math::cross(r, imp * sign);
        body->setAngularVelocity(body->getAngularVelocity() +
                                 applyWorldInvInertia(obj, invI, torque));
    };

    applyImpulse(objA, bodyA, -1.0f, impulseN, rA, invIA);
    applyImpulse(objB, bodyB, +1.0f, impulseN, rB, invIB);

    // Atrito de Coulomb na direção tangencial.
    relVel = pointVelocity(bodyB, rB) - pointVelocity(bodyA, rA);
    Vector3 tangent = relVel - normal * Yume::Math::dot(relVel, normal);
    float tLen = Yume::Math::length(tangent);
    if (tLen < 1e-6f)
        return;
    tangent = tangent * (1.0f / tLen);

    float denomT = invMassA + invMassB + angularTerm(objA, rA, invIA, tangent) +
                   angularTerm(objB, rB, invIB, tangent);
    if (denomT <= 1e-8f)
        return;

    float jt = -Yume::Math::dot(relVel, tangent) / denomT;
    float maxFriction = c.friction * jnForFriction;
    if (jt > maxFriction)
        jt = maxFriction;
    else if (jt < -maxFriction)
        jt = -maxFriction;

    Vector3 impulseT = tangent * jt;
    applyImpulse(objA, bodyA, -1.0f, impulseT, rA, invIA);
    applyImpulse(objB, bodyB, +1.0f, impulseT, rB, invIB);
}

void translateObject(WorldObject* obj, const Vector3& delta) {
    Transform& t = obj->getTransform();
    t.setPosition(t.getPosition() + delta);
}

/// @brief Separa posicionalmente os corpos de UM contato (anti-penetração).
void positionalCorrection(const Contact& c) {
    // Fator de folga (slop) e correção parcial para estabilidade do assentamento.
    const float slop = 0.005f;
    const float percent = 0.6f;
    float corr = (c.penetration - slop > 0.0f ? c.penetration - slop : 0.0f) * percent;
    if (corr <= 0.0f)
        return;

    float moveA = 0.0f, moveB = 0.0f;
    if (c.bodyA && c.bodyB) {
        moveA = 0.5f;
        moveB = 0.5f;
    } else if (c.bodyA) {
        moveA = 1.0f;
    } else if (c.bodyB) {
        moveB = 1.0f;
    }
    if (moveA > 0.0f)
        translateObject(c.objA, c.normal * (-corr * moveA));
    if (moveB > 0.0f)
        translateObject(c.objB, c.normal * (corr * moveB));
}

// -----------------------------------------------------------------------------
// Detecção: gera Contacts e os acumula.
// -----------------------------------------------------------------------------

float combinedRestitution(Rigidbody* a, Rigidbody* b) {
    return std::min(a ? a->getRestitution() : 0.0f, b ? b->getRestitution() : 0.0f);
}
float combinedFriction(Rigidbody* a, Rigidbody* b) {
    float fa = a ? a->getFriction() : 0.5f;
    float fb = b ? b->getFriction() : 0.5f;
    return 0.5f * (fa + fb);
}

/// @brief OBB vs plano: um contato por vértice abaixo do plano (manifold estável).
void collideOBBPlane(const OBB& o, const Vector3& n, const Vector3& planePoint,
                     std::vector<Contact>& out) {
    Vector3 verts[8];
    obbVertices(o, verts);
    for (int i = 0; i < 8; i++) {
        float dist = Yume::Math::dot(verts[i] - planePoint, n);
        if (dist < 0.0f) {
            Contact c;
            c.objA = nullptr;
            c.bodyA = nullptr; // plano estático é A
            c.objB = o.object;
            c.bodyB = o.body;
            c.normal = n; // de A (plano) para B (caixa)
            c.penetration = -dist;
            c.point = verts[i];
            c.restitution = combinedRestitution(nullptr, o.body);
            c.friction = combinedFriction(nullptr, o.body);
            out.push_back(c);
        }
    }
}

/// @brief Esfera vs plano.
void collideSpherePlane(const WorldSphere& s, const Vector3& n, const Vector3& planePoint,
                        std::vector<Contact>& out) {
    float penetration = s.radius - Yume::Math::dot(s.center - planePoint, n);
    if (penetration <= 0.0f)
        return;
    Contact c;
    c.objB = s.object;
    c.bodyB = s.body;
    c.normal = n;
    c.penetration = penetration;
    c.point = s.center - n * s.radius;
    c.restitution = combinedRestitution(nullptr, s.body);
    c.friction = combinedFriction(nullptr, s.body);
    out.push_back(c);
}

/// @brief Esfera vs OBB: ponto mais próximo no espaço LOCAL da caixa.
void collideSphereOBB(const WorldSphere& s, const OBB& o, std::vector<Contact>& out) {
    // Centro da esfera relativo ao centro da caixa, projetado nos eixos locais.
    Vector3 d = s.center - o.center;
    float localqx = Yume::Math::dot(d, o.axis[0]);
    float localqy = Yume::Math::dot(d, o.axis[1]);
    float localqz = Yume::Math::dot(d, o.axis[2]);
    float cx = std::max(-o.half.x, std::min(localqx, o.half.x));
    float cy = std::max(-o.half.y, std::min(localqy, o.half.y));
    float cz = std::max(-o.half.z, std::min(localqz, o.half.z));

    Vector3 closest = o.center + o.axis[0] * cx + o.axis[1] * cy + o.axis[2] * cz;
    Vector3 delta = s.center - closest;
    float distSq = Yume::Math::lengthSquared(delta);
    if (distSq >= s.radius * s.radius)
        return;

    float dist = std::sqrt(distSq);
    Vector3 normal; // de A (caixa) para B (esfera)
    float penetration;
    if (dist > 1e-5f) {
        normal = delta * (1.0f / dist);
        penetration = s.radius - dist;
    } else {
        // Centro da esfera dentro da caixa: empurra pela face local mais próxima.
        float px = o.half.x - std::fabs(localqx);
        float py = o.half.y - std::fabs(localqy);
        float pz = o.half.z - std::fabs(localqz);
        if (px <= py && px <= pz) {
            normal = o.axis[0] * (localqx < 0 ? -1.0f : 1.0f);
            penetration = s.radius + px;
        } else if (py <= px && py <= pz) {
            normal = o.axis[1] * (localqy < 0 ? -1.0f : 1.0f);
            penetration = s.radius + py;
        } else {
            normal = o.axis[2] * (localqz < 0 ? -1.0f : 1.0f);
            penetration = s.radius + pz;
        }
    }

    Contact c;
    c.objA = o.object;
    c.bodyA = o.body;
    c.objB = s.object;
    c.bodyB = s.body;
    c.normal = normal;
    c.penetration = penetration;
    c.point = closest;
    c.restitution = combinedRestitution(o.body, s.body);
    c.friction = combinedFriction(o.body, s.body);
    out.push_back(c);
}

/// @brief Projeção (raio) de uma OBB sobre um eixo unitário.
float obbProject(const OBB& o, const Vector3& axis) {
    return std::fabs(Yume::Math::dot(o.axis[0], axis)) * o.half.x +
           std::fabs(Yume::Math::dot(o.axis[1], axis)) * o.half.y +
           std::fabs(Yume::Math::dot(o.axis[2], axis)) * o.half.z;
}

/**
 * @brief OBB vs OBB por SAT (15 eixos). Gera UM contato no eixo de menor
 *        penetração — o solver iterativo cuida do assentamento com múltiplos
 *        passos. Normal orientada de A para B.
 */
void collideOBBOBB(const OBB& a, const OBB& b, std::vector<Contact>& out) {
    Vector3 t = b.center - a.center;

    float minPen = 1e30f;
    Vector3 bestAxis{};
    bool found = false;

    auto testAxis = [&](Vector3 axis) -> bool {
        float len = Yume::Math::length(axis);
        if (len < 1e-6f)
            return true; // eixo degenerado (faces paralelas): ignora.
        axis = axis * (1.0f / len);
        float ra = obbProject(a, axis);
        float rb = obbProject(b, axis);
        float dist = std::fabs(Yume::Math::dot(t, axis));
        float overlap = ra + rb - dist;
        if (overlap < 0.0f)
            return false; // eixo separador encontrado -> sem colisão.
        if (overlap < minPen) {
            minPen = overlap;
            // Normal sempre de A para B.
            bestAxis = (Yume::Math::dot(t, axis) < 0.0f) ? axis * -1.0f : axis;
            found = true;
        }
        return true;
    };

    // 3 eixos de face de A + 3 de B.
    for (int i = 0; i < 3; i++)
        if (!testAxis(a.axis[i]))
            return;
    for (int i = 0; i < 3; i++)
        if (!testAxis(b.axis[i]))
            return;
    // 9 cross-products (aresta-aresta).
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (!testAxis(Yume::Math::cross(a.axis[i], b.axis[j])))
                return;

    if (!found)
        return;

    // Ponto de contato aproximado: do centro de A, avança pela normal até a
    // superfície, e meia-penetração para dentro (ponto no meio da sobreposição).
    Vector3 support = a.center + bestAxis * (obbProject(a, bestAxis) - minPen * 0.5f);

    Contact c;
    c.objA = a.object;
    c.bodyA = a.body;
    c.objB = b.object;
    c.bodyB = b.body;
    c.normal = bestAxis;
    c.penetration = minPen;
    c.point = support;
    c.restitution = combinedRestitution(a.body, b.body);
    c.friction = combinedFriction(a.body, b.body);
    out.push_back(c);
}

/// @brief Executa um único sub-passo de física de duração @p h (segundos).
void substep(WorldObjectManager* manager, const Vector3& gravity, float linearDamping,
             float angularDamping, float maxAngularSpeed, float h);

} // namespace

void PhysicsWorld::step(WorldObjectManager* manager, float deltaTime) {
    if (!enabled || manager == nullptr || deltaTime <= 0.0f)
        return;

    // Sub-stepping de passo fixo: dividimos o dt do frame em fatias de no máximo
    // FIXED_STEP. Isso torna a simulação estável e independente do framerate — um
    // frame longo vira vários passos pequenos em vez de um salto grande (que
    // tunelaria/penetraria fundo). Combinado ao clamp do Timer, elimina o
    // "pulinho" ao arrastar a janela. Limitamos a contagem de sub-passos para
    // não entrar em espiral de morte se um frame vier muito longo.
    const float FIXED_STEP = 1.0f / 120.0f;
    const int MAX_SUBSTEPS = 8;

    int steps = static_cast<int>(deltaTime / FIXED_STEP);
    if (steps < 1)
        steps = 1;
    if (steps > MAX_SUBSTEPS)
        steps = MAX_SUBSTEPS;
    float h = deltaTime / static_cast<float>(steps);

    for (int i = 0; i < steps; i++)
        substep(manager, gravity, linearDamping, angularDamping, maxAngularSpeed, h);
}

namespace {

void substep(WorldObjectManager* manager, const Vector3& gravity, float linearDamping,
             float angularDamping, float maxAngularSpeed, float deltaTime) {
    // 1) Integração: gravidade, movimento linear, rotação (quatérnion) e damping.
    for (auto& obj : manager->getObjects()) {
        Rigidbody* body = obj->getComponent<Rigidbody>();
        if (!body)
            continue;

        setupInertia(obj.get(), body);

        Vector3 velocity = body->getVelocity();
        if (body->getUseGravity())
            velocity = velocity + gravity * (body->getGravityScale() * deltaTime);

        float linFactor = std::pow(1.0f - linearDamping, deltaTime);
        float angFactor = std::pow(1.0f - angularDamping, deltaTime);
        velocity = velocity * linFactor;
        body->setVelocity(velocity);

        Vector3 w = body->getAngularVelocity() * angFactor;
        float wLen = Yume::Math::length(w);
        if (wLen > maxAngularSpeed)
            w = w * (maxAngularSpeed / wLen);
        body->setAngularVelocity(w);

        Transform& t = obj->getTransform();
        t.setPosition(t.getPosition() + velocity * deltaTime);
        if (Yume::Math::lengthSquared(w) > 0.0f)
            t.setRotationQuat(Yume::Math::integrate(t.getRotationQuat(), w, deltaTime));
    }

    // 2) Monta os colisores em mundo (OBB orientada / esfera).
    std::vector<OBB> obbs;
    std::vector<WorldSphere> spheres;
    for (auto& obj : manager->getObjects()) {
        if (BoxCollider* bc = obj->getComponent<BoxCollider>())
            obbs.push_back(makeOBB(obj.get(), bc));
        if (SphereCollider* sc = obj->getComponent<SphereCollider>()) {
            WorldSphere ws;
            ws.object = obj.get();
            ws.body = obj->getComponent<Rigidbody>();
            ws.center = colliderCenter(obj.get(), sc->getCenter());
            ws.radius = sc->getRadius();
            spheres.push_back(ws);
        }
    }

    // 3) Detecção: coleta todos os contatos num único manifold da cena.
    std::vector<Contact> contacts;

    for (auto& obj : manager->getObjects()) {
        PlaneCollider* plane = obj->getComponent<PlaneCollider>();
        if (!plane)
            continue;
        Vector3 n = Yume::Math::normalize(plane->getNormal());
        Vector3 planePoint = obj->getTransform().getPosition() + n * plane->getOffset();
        for (auto& o : obbs)
            if (o.body)
                collideOBBPlane(o, n, planePoint, contacts);
        for (auto& s : spheres)
            if (s.body)
                collideSpherePlane(s, n, planePoint, contacts);
    }

    for (size_t i = 0; i < obbs.size(); i++)
        for (size_t j = i + 1; j < obbs.size(); j++)
            if (obbs[i].body || obbs[j].body)
                collideOBBOBB(obbs[i], obbs[j], contacts);

    for (size_t i = 0; i < spheres.size(); i++)
        for (size_t j = i + 1; j < spheres.size(); j++) {
            WorldSphere& a = spheres[i];
            WorldSphere& b = spheres[j];
            if (!a.body && !b.body)
                continue;
            Vector3 d = b.center - a.center;
            float distSq = Yume::Math::lengthSquared(d);
            float sum = a.radius + b.radius;
            if (distSq >= sum * sum)
                continue;
            float dist = std::sqrt(distSq);
            Vector3 normal = (dist > 1e-4f) ? d * (1.0f / dist) : Vector3{0.0f, 1.0f, 0.0f};
            Contact c;
            c.objA = a.object;
            c.bodyA = a.body;
            c.objB = b.object;
            c.bodyB = b.body;
            c.normal = normal;
            c.penetration = sum - dist;
            c.point = a.center + normal * (a.radius - c.penetration * 0.5f);
            c.restitution = combinedRestitution(a.body, b.body);
            c.friction = combinedFriction(a.body, b.body);
            contacts.push_back(c);
        }

    for (auto& s : spheres)
        for (auto& o : obbs)
            if (s.body || o.body)
                collideSphereOBB(s, o, contacts);

    // 4) Resolução iterativa das velocidades (sequential impulses). Várias
    //    passadas sobre o mesmo conjunto de contatos deixam o cubo assentar
    //    estável (um ponto só por par tremeria).
    const int velocityIterations = 8;
    for (int it = 0; it < velocityIterations; it++)
        for (auto& c : contacts)
            resolveContactVelocity(c);

    // 5) Correção posicional (anti-penetração) depois das velocidades.
    for (auto& c : contacts)
        positionalCorrection(c);
}

} // namespace
