/**
 * SDL window creation adapted from https://github.com/isJuhn/DoublePendulum
*/
#include "simulate.h"
#include "thread"
#include "time.h"
Eigen::MatrixXf LQR(PlanarQuadrotor &quadrotor, float dt) {
    /* Calculate LQR gain matrix */
    Eigen::MatrixXf Eye = Eigen::MatrixXf::Identity(6, 6);
    Eigen::MatrixXf A = Eigen::MatrixXf::Zero(6, 6);
    Eigen::MatrixXf A_discrete = Eigen::MatrixXf::Zero(6, 6);
    Eigen::MatrixXf B(6, 2);
    Eigen::MatrixXf B_discrete(6, 2);
    Eigen::MatrixXf Q = Eigen::MatrixXf::Identity(6, 6);
    Eigen::MatrixXf R = Eigen::MatrixXf::Identity(2, 2);
    Eigen::MatrixXf K = Eigen::MatrixXf::Zero(6, 6);
    Eigen::Vector2f input = quadrotor.GravityCompInput();

    Q.diagonal() << 4e-3, 4e-3, 4e2, 8e-3, 4.5e-2, 2 / 2 / M_PI;
    R.row(0) << 3e1, 7;
    R.row(1) << 7, 3e1;

    std::tie(A, B) = quadrotor.Linearize();
    A_discrete = Eye + dt * A;
    B_discrete = dt * B;
    
    return LQR(A_discrete, B_discrete, Q, R);
}

void control(PlanarQuadrotor &quadrotor, const Eigen::MatrixXf &K) {
    Eigen::Vector2f input = quadrotor.GravityCompInput();
    quadrotor.SetInput(input - K * quadrotor.GetControlState());
}

int main(int argc, char* args[])
{
    std::shared_ptr<SDL_Window> gWindow = nullptr;
    std::shared_ptr<SDL_Renderer> gRenderer = nullptr;
    const int SCREEN_WIDTH = 1280;
    const int SCREEN_HEIGHT = 720;

    /**
     * TODO: Extend simulation
     * 1. Set goal state of the mouse when clicking left mouse button (transform the coordinates to the quadrotor world! see visualizer TODO list)
     *    [x, y, 0, 0, 0, 0]
     * 2. Update PlanarQuadrotor from simulation when goal is changed
    */
    srand(time(NULL));
    int init_x = rand()%(SCREEN_WIDTH) + 1;
    int init_y = rand()%(SCREEN_HEIGHT) + 1;
    Eigen::VectorXf initial_state = Eigen::VectorXf::Zero(6);
    initial_state<<init_x, init_y, 0, 0, 0, 0;
    PlanarQuadrotor quadrotor(initial_state);
    PlanarQuadrotorVisualizer quadrotor_visualizer(&quadrotor);
    /**
     * Goal pose for the quadrotor
     * [x, y, theta, x_dot, y_dot, theta_dot]
     * For implemented LQR controller, it has to be [x, y, 0, 0, 0, 0]
    */
    Eigen::VectorXf goal_state = Eigen::VectorXf::Zero(6);
    goal_state<<SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 0, 0, 0, 0;
    quadrotor.SetGoal(goal_state);
    /* Timestep for the simulation */
    const float dt = 0.01;
    Eigen::MatrixXf K = LQR(quadrotor, dt);
    Eigen::Vector2f input = Eigen::Vector2f::Zero(2);

    /**
     * TODO: Plot x, y, theta over time
     * 1. Update x, y, theta history vectors to store trajectory of the quadrotor
     * 2. Plot trajectory using matplot++ when key 'p' is clicked
    */
    std::vector<float> x_history;
    std::vector<float> y_history;
    std::vector<float> theta_history;
    float time = 0;
    if (init(gWindow, gRenderer, SCREEN_WIDTH, SCREEN_HEIGHT) >= 0)
    {
        
        SDL_Event e;
        bool quit = false;
        float delay;
        int x, y;
        Eigen::VectorXf state = Eigen::VectorXf::Zero(6);
        
        while (!quit)
        {
            //events
            bool check = false;
            while (SDL_PollEvent(&e) != 0)
            {
                if (e.type == SDL_QUIT)
                {
                    quit = true;
                }
                // else if (e.type == SDL_MOUSEMOTION)
                // {
                //     SDL_GetMouseState(&x, &y);
                //     //std::cout << "Mouse position: (" << x << ", " << y << ")" << std::endl;
                // }
                else if (e.type == SDL_MOUSEBUTTONDOWN)
                {
                    SDL_GetMouseState(&x, &y);
                    std::cout<<"Goal: ("<<x<<", "<<y<<")"<<std::endl;
                    goal_state <<x ,y ,0 ,0 ,0 ,0;
                    quadrotor.SetGoal(goal_state);
                    check = true;

                }
                else if(e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_p){
                    std::thread t(PlanarQuadrotor::PlotHistory,quadrotor);
                    t.detach();
                }
            }

            SDL_Delay((int) dt * 1000);

            SDL_SetRenderDrawColor(gRenderer.get(), 0xFF, 0xFF, 0xFF, 0xFF);
            SDL_RenderClear(gRenderer.get());

            /* Quadrotor rendering step */
            quadrotor_visualizer.render(gRenderer);

            SDL_RenderPresent(gRenderer.get());
            time +=dt;
            /* Simulate quadrotor forward in time */
            control(quadrotor, K);
            quadrotor.Update(dt,time);
        }
    }
    SDL_Quit();
    return 0;
}

int init(std::shared_ptr<SDL_Window>& gWindow, std::shared_ptr<SDL_Renderer>& gRenderer, const int SCREEN_WIDTH, const int SCREEN_HEIGHT)
{
    if (SDL_Init(SDL_INIT_VIDEO) >= 0)
    {
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        gWindow = std::shared_ptr<SDL_Window>(SDL_CreateWindow("Planar Quadrotor", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN), SDL_DestroyWindow);
        gRenderer = std::shared_ptr<SDL_Renderer>(SDL_CreateRenderer(gWindow.get(), -1, SDL_RENDERER_SOFTWARE), SDL_DestroyRenderer);
        SDL_SetRenderDrawColor(gRenderer.get(), 0xFF, 0xFF, 0xFF, 0xFF);
    }
    else
    {
        std::cout << "SDL_ERROR: " << SDL_GetError() << std::endl;
        return -1;
    }
    return 0;
}