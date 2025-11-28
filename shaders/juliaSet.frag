#version 430

uniform vec3 camPos;
uniform vec3 camLookAt;
uniform vec3 camUp;
uniform float fov;
uniform vec2 resolution;
uniform mat3 rotation;
// uniform vec4 juliaConstant; // c in z = z^2 + c as a quaternion 
// uniform int maxSteps;       // max sphere trace iterations for raymarching
uniform float time;

vec3 camRight;

// julia set is centered at origin, encapsulated by bounding sphere
const float BOUNDING_SPHERE_RADIUS = 2.0;
// const float EPSILON = 1e-2;
// const float EPSILON = 0.002;
// const int AASAMPLES = 4;
// const int maxSteps = 80;
const float ESCAPE_THRESHOLD = 1e1;
const float DELTA = 1e-4; // used in finite difference approximation of the gradient to determine normals  

uniform int AASAMPLES;
uniform vec4 juliaConstant;
uniform int maxSteps;
uniform float EPSILON;

// const vec4 juliaConstant = vec4(-0.04, 0.95, 0.4, -0.43);
// const vec4 juliaConstant = vec4( 0.15, -0.85,  0.50, -0.20);
// const vec4 juliaConstant = vec4(-0.45,  0.80,  0.15,  0.30);
// const vec4 juliaConstant = vec4( 0.50,  0.20, -0.75,  0.25);
// const vec4 juliaConstant = vec4(-0.60, -0.20,  0.80, -0.10);
// const vec4 juliaConstant = vec4(0.1, -0.5, 0.6, 0.0);

struct Ray
{
    vec3 dir;
    vec3 origin;
};

vec4 quartMult(vec4 q1, vec4 q2)
{
    vec4 a;
    a.x = q1.x * q2.x - dot(q1.yzw, q2.yzw);
    a.yzw = q1.x * q2.yzw + q2.x * q1.yzw + cross(q1.yzw, q2.yzw);
    return a;
}

vec4 quartSquared(vec4 q)
{
    vec4 a;
    a.x = q.x * q.x - dot(q.yzw, q.yzw);
    a.yzw = 2.0 * q.x * q.yzw;
    return a;
}

// to move the ray onto the sphere bounding the julia set before starting raymarching
float intersectBoundingSphere(vec3 r0, vec3 rd)
{
    float B = 2.0 * dot(r0, rd);
    float C = dot(r0, r0) - BOUNDING_SPHERE_RADIUS*BOUNDING_SPHERE_RADIUS;

    float disc = B*B - 4.0*C;
    if (disc < 0.0) return -1.0;

    float s = sqrt(disc);
    float t0 = (-B - s) * 0.5;
    float t1 = (-B + s) * 0.5;

    float t = (t0 > 0.0) ? t0 : t1;
    if (t < 0.0) return -1.0;

    return t;
}

void iterateIntersect(inout vec4 q, inout vec4 qp)
{
    for (int i = 0; i < maxSteps; i++)
    {
        qp = 2.0 * quartMult(q, qp);
        q = quartSquared(q) + juliaConstant;

        if (dot(q,q) > ESCAPE_THRESHOLD)
        {
            break;
        }
    }
}

// given a point, get the distance to julia set
float distanceEstimate(inout Ray r)
{
    float dist;

    while (true)
    {
        vec4 z = vec4(rotation * r.origin, 0.0);
        vec4 zp = vec4(1.0, 0.0, 0.0, 0.0);

        iterateIntersect(z, zp);

        // find lower bound on dist to julia set
        float normZ = length(z);
        float d = max(length(zp), 1e-6);
        dist = 0.5 * normZ * log(normZ) / d;
        // dist = 0.5 * normZ * log(normZ) / length(zp);

        r.origin += r.dir * dist;

        if (dist < EPSILON || dot(r.origin, r.origin) > BOUNDING_SPHERE_RADIUS * BOUNDING_SPHERE_RADIUS)
        {
            break;
        }
    }

    return dist;
}

float deAt(vec3 p)
{
    Ray r;
    r.origin = p;
    r.dir = vec3(1,0,0);
    return distanceEstimate(r);
}

vec3 estimateNorm(vec3 p)
{
    const float e = 0.001;

    float dx = deAt(p + vec3(e, 0, 0)) - deAt(p - vec3(e, 0, 0));
    float dy = deAt(p + vec3(0, e, 0)) - deAt(p - vec3(0, e, 0));
    float dz = deAt(p + vec3(0, 0, e)) - deAt(p - vec3(0, 0, e));

    return normalize(vec3(dx, dy, dz));
}



// vec3 estimateNorm(vec3 p)
// {
//     vec3 N;
//     vec4 qP = vec4(p, 0.0);

//     float gradX, gradY, gradZ;

//     vec4 gx1 = qP - vec4(DELTA, 0.0, 0.0, 0.0);
//     vec4 gx2 = qP - vec4(DELTA, 0.0, 0.0, 0.0);
//     vec4 gy1 = qP - vec4(0.0, DELTA, 0.0, 0.0);
//     vec4 gy2 = qP - vec4(0.0, DELTA, 0.0, 0.0);
//     vec4 gz1 = qP - vec4(0.0, 0.0, DELTA, 0.0);
//     vec4 gz2 = qP - vec4(0.0, 0.0, DELTA, 0.0);

//     for (int i = 0; i < maxSteps; i++)
//     {
//         gx1 = quartSquared(gx1) + juliaConstant;
//         gx2 = quartSquared(gx2) + juliaConstant;
//         gy1 = quartSquared(gy1) + juliaConstant;
//         gy2 = quartSquared(gy2) + juliaConstant;
//         gz1 = quartSquared(gz1) + juliaConstant;
//         gz2 = quartSquared(gz2) + juliaConstant;
//     }

//     gradX = length(gx2) - length(gx1);
//     gradY = length(gy2) - length(gy1);
//     gradZ = length(gz2) - length(gz1);
//     N = normalize(vec3(gradX, gradY, gradZ));
//     return N;
// }

vec3 shadePhong(vec3 L, vec3 P, vec3 N)
{
    vec3 diffuse = vec3(0.0, 1.0, 0.25); // base color - make this an input
    const int specExp = 10;
    const float specularity = 0.45;

    vec3 light = normalize(L - P);
    vec3 eye = normalize(camPos - P);
    float nDotL = dot(N, light);
    vec3 R = light - 2.0 * nDotL * N;

    diffuse += abs(N) * 0.3; // add the normal to color for the lolz

    return diffuse * max(nDotL, 0.0) + specularity * pow(max(dot(eye, R), 0.0), specExp);
}

in vec2 UV;

out vec4 color;

void main() 
{
    vec4 bgCol = vec4(0.5, 0.5, 0.5, 1.0);
    color = bgCol; // no intersection, background color

    // Compute camera basis
	vec3 camRight = normalize(cross(camLookAt, camUp));

    // Compute NDC coords
    float aR = resolution.x / resolution.y;
    float w2 = resolution.x / 2.0f;
    float h2 = resolution.y / 2.0f;
    float focal = 1.0 / tan(radians(fov) * 0.5);
    // vec2 ndc = UV * 2.0 - 1.0;

    vec3 finalCol = vec3(0.0);
    for (int s = 0; s < AASAMPLES; s++)
    {
        // jitter inside pixel
        vec2 jitter = vec2(
            fract(sin(dot(UV, vec2(12.9898, 78.233)) + float(s)) * 43758.5453),
            fract(sin(dot(UV, vec2(39.3461, 11.135)) + float(s)) * 91173.1224)
        );

        vec2 uvJ = UV + (jitter - 0.5) / resolution;
        vec2 ndc = uvJ * 2.0 - 1.0;
        vec3 target = camPos 
                    + focal * camLookAt
                    + ndc.x * aR * camRight  
                    + ndc.y * camUp;

        Ray ray;
        ray.dir = normalize(target - camPos);
        // ray.dir = normalize(rotation * ray.dir);
        ray.origin = camPos;

        float t = intersectBoundingSphere(ray.origin, ray.dir);
        if (t > 0.0)
        {
            // move ray onto bounding sphere
            ray.origin += ray.dir * t;
            // color = vec4(ray.dir, 1.0);
            
            float dist = distanceEstimate(ray);
            if (dist <= EPSILON)
            {
                // estimate the surface normal at this hit point
                vec3 norm = estimateNorm(ray.origin);

                // color = vec4(norm, 1.0);
                vec3 light = vec3(0.0, 0.0, 5.0);
                // color = vec4(norm, 1.0);
                finalCol += shadePhong(light, ray.origin, norm);
                // color = vec4(rotation[0].x, rotation[1].x, rotation[2].x, 1.0);
            }
            else
            {
                finalCol += vec3(0.5);
            }
        }
        else
        {
            // no hit with fractal
            finalCol += vec3(0.5);
        }
    }

    finalCol /= float(AASAMPLES);
    color = vec4(finalCol, 1.0);
}