//
// Created by Göksu Güvendiren on 2019-05-14.
//

#include "Scene.hpp"


void Scene::buildBVH() {
    printf(" - Generating BVH...\n\n");
    this->bvh = new BVHAccel(objects, 1, BVHAccel::SplitMethod::NAIVE);
}

Intersection Scene::intersect(const Ray &ray) const
{
    return this->bvh->Intersect(ray);
}

void Scene::sampleLight(Intersection &pos, float &pdf) const
{
    float emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
        }
    }
    float p = get_random_float() * emit_area_sum;
    emit_area_sum = 0;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        if (objects[k]->hasEmit()){
            emit_area_sum += objects[k]->getArea();
            if (p <= emit_area_sum){
                objects[k]->Sample(pos, pdf);
                break;
            }
        }
    }
}

bool Scene::trace(
        const Ray &ray,
        const std::vector<Object*> &objects,
        float &tNear, uint32_t &index, Object **hitObject)
{
    *hitObject = nullptr;
    for (uint32_t k = 0; k < objects.size(); ++k) {
        float tNearK = kInfinity;
        uint32_t indexK;
        Vector2f uvK;
        if (objects[k]->intersect(ray, tNearK, indexK) && tNearK < tNear) {
            *hitObject = objects[k];
            tNear = tNearK;
            index = indexK;
        }
    }


    return (*hitObject != nullptr);
}

Vector3f Scene::shade(Intersection& hit_obj, Vector3f wo) const { 
    /**
     * @brief 对于物体上一个点的一个出射方向的颜色
     * hit_obj: 相交点
     * wo: 出射方向
     */

    //如果物体会发光，直接返回光照
    if (hit_obj.m->hasEmission())
    {
        return hit_obj.m->getEmission();
    }
    const float epsilon = 0.0005f;
    const float pdf_epsilon = 0.000001f;
    //直接光照
    Vector3f Lo_direct = {0,0,0}; // 直接光照的贡献
    Intersection light_intersection;
    float light_pdf;
    //对光源进行采样
    sampleLight(light_intersection, light_pdf);
    //查看直接光照是否有障碍物遮挡
    Vector3f hit_obj_to_light= (light_intersection.coords - hit_obj.coords); //物体的击中点到光源的向量(没有归一化)
    Vector3f hit_obj_to_light_dir  = hit_obj_to_light.normalized(); //物体的击中点到光源的方向向量(归一化)
    //这里需要利用光的传播距离和物体到光源的距离来判断是否 *中间* 有物体存在，而不能直接使用.happend来判断是否有交点，因为.happend不能保证交点在中间
    Intersection t = intersect(Ray(hit_obj.coords, hit_obj_to_light_dir));
    if(t.distance - hit_obj_to_light.norm() > -epsilon){
        //光线没有被遮挡
        Vector3f f_r = hit_obj.m->eval(-hit_obj_to_light_dir, wo, hit_obj.normal);
        //因为是直接对光源采样，所以需要改写公式
        float distance = dotProduct(hit_obj_to_light, hit_obj_to_light);
        float cosine1 = dotProduct(hit_obj.normal, hit_obj_to_light_dir);
        float cosine2 = dotProduct(light_intersection.normal, -hit_obj_to_light_dir);
        if(light_pdf > pdf_epsilon)
            Lo_direct = light_intersection.emit * f_r * cosine1*cosine2 / light_pdf / distance;
    }

    //间接光照
    Vector3f Lo_indirect = {0,0,0}; //间接光照的贡献
    //俄罗斯轮盘赌
    if(get_random_float() < RussianRoulette){
        //重新采样
        Vector3f obj1_to_obj2_dir = hit_obj.m->sample(-wo, hit_obj.normal).normalized();
        float obj2_pdf = hit_obj.m->pdf(-wo,obj1_to_obj2_dir,hit_obj.normal);
        Intersection t_2 = intersect(Ray(hit_obj.coords, obj1_to_obj2_dir));
        if(t_2.happened && !t_2.m->hasEmission()){ //这里需要确定交点处不会发光，来确保这是间接光照而不是直接射到了光源
            //光线和物体相交
            Vector3f f_r = hit_obj.m->eval(-obj1_to_obj2_dir, wo, hit_obj.normal);
            float cosine = dotProduct(hit_obj.normal, obj1_to_obj2_dir); 
            if(obj2_pdf > pdf_epsilon)
                Lo_indirect = shade(t_2, -obj1_to_obj2_dir) * f_r * cosine / obj2_pdf / RussianRoulette;
        }
    }
    return Lo_direct + Lo_indirect;
}

// Implementation of Path Tracing
Vector3f Scene::castRay(const Ray &ray, int depth) const
{
    // TO DO Implement Path Tracing Algorithm here
    Intersection intersection = intersect(ray);
    if(intersection.happened){
        return shade(intersection,-ray.direction);
    }
    return {0,0,0};
}