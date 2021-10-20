#include "Triangle.hpp"
#include "rasterizer.hpp"
#include <eigen3/Eigen/Eigen>
#include <iostream>
#include <opencv2/opencv.hpp>

constexpr double MY_PI = 3.1415926;

Eigen::Matrix4f get_view_matrix(Eigen::Vector3f eye_pos)
{
    Eigen::Matrix4f view = Eigen::Matrix4f::Identity();

    Eigen::Matrix4f translate;
    translate << 1, 0, 0, -eye_pos[0], 0, 1, 0, -eye_pos[1], 0, 0, 1,
        -eye_pos[2], 0, 0, 0, 1;

    view = translate * view;

    return view;
}


Eigen::Matrix4f get_model_matrix(float rotation_angle)
{
    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the model matrix for rotating the triangle around the Z axis.
    // Then return it.
    model(0,0) = cos( (rotation_angle/180) * MY_PI);
    model(0,1) = (-1) * sin( (rotation_angle/180) * MY_PI);
    model(1,0) = sin( (rotation_angle/180) * MY_PI);
    model(1,1) = cos( (rotation_angle/180) * MY_PI);

    return model;
}


//bonus. ratation around any given axis
Eigen::Matrix4f get_rotation(Vector3f axis, float rotation_angle)
{

    Eigen::Matrix4f model = Eigen::Matrix4f::Identity();
    //rodrigues rotation fomula
    Eigen::Matrix3f rodrigues = Eigen::Matrix3f::Identity();

    float alpha = (rotation_angle/180) * MY_PI;
    Eigen::Matrix3f N = Eigen::Matrix3f::Zero();
    N(0,1) = -1*(axis[2]); N(0,2) = axis[1]; 
    N(1,0) = axis[2];      N(1,2) = -1*(axis[0]); 
    N(2,0) = -1*(axis[1]); N(2,1) = axis[0]; 


    Eigen::Matrix3f part1 = cos(alpha)*Eigen::Matrix3f::Identity();
    Eigen::Matrix3f part2 = (1-cos(alpha))* (axis * axis.transpose());
    Eigen::Matrix3f part3 = sin(alpha) * N;

    
    rodrigues = part1 + part2 + part3;

    for(int i=0;i<3;i++){
        for(int j=0;j<3;j++){
            model(i,j) = rodrigues(i,j);
        }
    }

    return model;

}


Eigen::Matrix4f get_projection_matrix(float eye_fov, float aspect_ratio,
                                      float zNear, float zFar)
{
    // Students will implement this function

    Eigen::Matrix4f projection = Eigen::Matrix4f::Identity();

    // TODO: Implement this function
    // Create the projection matrix for the given parameters.
    // Then return it.

    float n = zNear;
    float f = zFar;
    float t = tan( (eye_fov/360)*MY_PI ) * abs(n);
    float b = t * (-1);
    float r = aspect_ratio * t;
    float l = r * (-1);

    // matric for orthographic
    Eigen::Matrix4f ortho_scale = Eigen::Matrix4f::Identity();
    Eigen::Matrix4f ortho_trans = Eigen::Matrix4f::Identity();
    ortho_scale(0,0) = 2/(r-l);
    ortho_scale(1,1) = 2/(t-b);
    ortho_scale(2,2) = 2/(n-f);

    ortho_trans(0,3) = (-1) * (r+l)/2;
    ortho_trans(1,3) = (-1) * (t+b)/2;
    ortho_trans(2,3) = (-1) * (n+f)/2;

    Eigen::Matrix4f ortho = ortho_scale * ortho_trans;

    // persp -> ortho matrix
    Eigen::Matrix4f ptom = Eigen::Matrix4f::Zero();
    ptom(0,0) = n; 
    ptom(1,1) = n;
    ptom(3,2) = 1;
    ptom(2,2) = n+f;
    ptom(2,3) = (-1)*n*f;


    // cal projection matrix
    projection = ortho * ptom;

    return projection;
}

int main(int argc, const char** argv)
{
    float angle = 0;
    bool command_line = false;
    std::string filename = "output.png";

    if (argc >= 3) {
        command_line = true;
        angle = std::stof(argv[2]); // -r by default
        if (argc == 4) {
            filename = std::string(argv[3]);
        }
        else
            return 0;
    }

    rst::rasterizer r(700, 700);

    Eigen::Vector3f eye_pos = {0, 0, 5};

    std::vector<Eigen::Vector3f> pos{{2, 0, -2}, {0, 2, -2}, {-2, 0, -2}};

    std::vector<Eigen::Vector3i> ind{{0, 1, 2}};

    auto pos_id = r.load_positions(pos);
    auto ind_id = r.load_indices(ind);

    int key = 0;
    int frame_count = 0;

    if (command_line) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);

        r.set_model(get_rotation({0,1.5,1},angle));
        //r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);
        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);

        cv::imwrite(filename, image);

        return 0;
    }

    while (key != 27) {
        r.clear(rst::Buffers::Color | rst::Buffers::Depth);
        r.set_model(get_rotation({0,0,1},angle));
        //r.set_model(get_model_matrix(angle));
        r.set_view(get_view_matrix(eye_pos));
        r.set_projection(get_projection_matrix(45, 1, 0.1, 50));

        r.draw(pos_id, ind_id, rst::Primitive::Triangle);

        cv::Mat image(700, 700, CV_32FC3, r.frame_buffer().data());
        image.convertTo(image, CV_8UC3, 1.0f);
        cv::imshow("image", image);
        key = cv::waitKey(10);

        std::cout << "frame count: " << frame_count++ << '\n';

        if (key == 'a') {
            angle += 10;
        }
        else if (key == 'd') {
            angle -= 10;
        }
    }

    return 0;
}
