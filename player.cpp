#include "player.hpp"
#include "engine.hpp"
#include <stdlib.h>
#include <chrono>
#include <cstring>
#include <stdio.h>
#include <ctype.h>
#include <map>
#include <math.h>

int my_max(int a, int b) 
{
    int diff = a - b;
    int dsgn = diff >> 31;
    return a - (diff & dsgn);
}
int my_min(int a, int b) 
{
    int diff = a - b;
    int dsgn = diff >> 31;
    return b + (diff & dsgn);
}

std::chrono::duration<double, std::nano> cast_nano2(std::chrono::duration<double> x)
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(x);
}

// PLAYER


Player::Player(int col, Engine* engine)
{
    color = col;
    e = engine;
}

int Player::get_color()
{
    return this->color;
}


//
// RAND
//

Rand::Rand(int col, Engine* engine) : Player(col, engine)
{
    //
}

int Rand::move(int* move_list)
{
    // int num_moves = move_list[0];
    if(move_list[0] == 0)
    {
        return -1;
    }
    int move_choice = rand() % move_list[0];
    move_list++;

    return move_list[move_choice];
}

void Rand::cleanup()
{
    //
}

//
// HUMAN
//

Human::Human(int col, Engine* engine) : Player(col, engine)
{
    //
}

int Human::parse_coords(std::string seq)
{
    std::map<char, int> char_mapper;
    char_mapper['A'] = 0;
    char_mapper['B'] = 1;
    char_mapper['C'] = 2;
    char_mapper['D'] = 3;
    char_mapper['E'] = 4;
    char_mapper['F'] = 5;
    char_mapper['G'] = 6;
    char_mapper['H'] = 7;

    // im dumb
    std::map<char, int> int_mapper;
    int_mapper['0'] = 0;
    int_mapper['1'] = 1;
    int_mapper['2'] = 2;
    int_mapper['3'] = 3;
    int_mapper['4'] = 4;
    int_mapper['5'] = 5;
    int_mapper['6'] = 6;
    int_mapper['7'] = 7;
    int_mapper['8'] = 8;

    char first = toupper(seq[0]);
    char second = seq[1];

    int first_int = char_mapper[first];
    int second_int = int_mapper[second];

    return first_int + second_int * 8;
}

int Human::move(int* move_list)
{
    if(move_list[0] == 0)
    {
        std::cout << e->color_to_string(get_color()) << " passed because they have no moves (human)" << std::endl;
        return -1;
    }

    int chosen_move = -1;
    int attempt_move;
    std::string input;
    while(chosen_move == -1)
    {
        std::cout << e->color_to_string(get_color()) << " enter your move" << std::endl;
        std::cin >> input;
        try
        {
            attempt_move = parse_coords(input);
        }
        catch(std::string input)
        {
            std::cout << input << ": was not a valid string to enter" << std::endl;
        }

        for(int i = 0; i < move_list[0]; i++)
        {
            if(attempt_move == move_list[i+1])
            {
                chosen_move = attempt_move;
            }
        }
        if(chosen_move == -1)
        {
            printf("%i was not a valid move\n", attempt_move);
        }
    }

    return chosen_move;
}

void Human::cleanup()
{
    //
}

//
// Minimax
//

Minimax::Minimax(int col, Engine* engine, int search_limit) : Player(col, engine)
{
    depth_search_limit = search_limit;
}

int Minimax::minimax_white(int depth, double alpha, double beta)
{
    node_count++;
    int* move_list = e->generate_white_moves();

    if(depth < 1 || e->is_terminal(move_list, color))
    {
        return e->score_board();
    }
    
    int num_moves = move_list[0];
    int* copied_move_list = copy_move_list(move_list);

    int value;
    int bestVal = -100000;
    for(int i = 0; i < num_moves; i++)
    {
        e->push_white_move(copied_move_list[i]);
        value = minimax_black(depth-1, alpha, beta);
        e->pop_move();

        bestVal = my_max(bestVal, value);
        alpha = my_max(alpha, bestVal);
        if(beta <= alpha)
        {
            free(copied_move_list);
            return bestVal;
        }
    }
    free(copied_move_list);

    if(num_moves == 0)
    {
        e->push_white_move(-1);
        bestVal = minimax_black(depth-1, alpha, beta);
        e->pop_move();
    }

    return bestVal;
}

int Minimax::minimax_black(int depth, double alpha, double beta)
{
    node_count++;
    int* move_list = e->generate_black_moves();
    
    if(depth < 1 || e->is_terminal(move_list, color))
    {
        return e->score_board();
    }
    
    int num_moves = move_list[0];
    int* copied_move_list = copy_move_list(move_list);

    int value;
    int bestVal = 100000;
    
    for(int i = 0; i < num_moves; i++)
    {
        e->push_black_move(copied_move_list[i]);
        value = minimax_white(depth-1, alpha, beta);
        e->pop_move();

        bestVal = my_min(bestVal, value);
        beta = my_min(beta, bestVal);
        if(beta <= alpha)
        {
            free(copied_move_list);
            return bestVal;
        }
    }
    free(copied_move_list);

    if(num_moves == 0)
    {
        e->push_black_move(-1);
        bestVal = minimax_white(depth-1, alpha, beta);
        e->pop_move();
    }

    return bestVal;
}


int Minimax::move(int* move_list)
{
    if(move_list[0] == 0)
    {
        // std::cout << e->color_to_string(get_color()) << " passed because they have no moves (minimax)" << std::endl;
        return -1;
    }


    //timing
    // std::chrono::time_point<std::chrono::system_clock> t1, t2;
    // std::chrono::duration<double, std::nano> time_cast_result;
    // t1 = std::chrono::system_clock::now();
    //end timing

    node_count = 0;

    int num_moves = move_list[0];
    int* copied_move_list = copy_move_list(move_list);


    int best_score;
    int best_move = copied_move_list[0];
    int temp;

    if(color)
    {
        e->push_white_move(copied_move_list[0]);
        best_score = minimax_black(depth_search_limit, -100000, 100000);
        e->pop_move();

        // printf("MINIMAXING FOR WHITE num_moves %i\n", num_moves);
        for(int i = 1; i < num_moves; i++)
        {
            e->push_white_move(copied_move_list[i]);
            temp = minimax_black(depth_search_limit, -100000, 100000);
            e->pop_move();
            if(temp > best_score)
            {
                best_score = temp;
                best_move = copied_move_list[i];
            }
        }
    }
    else
    {
        e->push_black_move(copied_move_list[0]);
        best_score = minimax_white(depth_search_limit, -100000, 100000);
        e->pop_move();

        // printf("MINIMAXING FOR BLACK\n");
        for(int i = 1; i < num_moves; i++)
        {
            e->push_black_move(copied_move_list[i]);
            temp = minimax_white(depth_search_limit, -100000, 100000);
            e->pop_move();
            if(temp < best_score)
            {
                best_score = temp;
                best_move = copied_move_list[i];
            }
        }
    }
    free(copied_move_list);

    //timing
    // t2 = std::chrono::system_clock::now();
    // time_cast_result = cast_nano2(t2 - t1);
    // double timing_temp = (double) time_cast_result.count() / node_count;

    // std::cout << "total nodes examined: " << node_count << " with " << timing_temp << " nanoseconds per move" << std::endl;
    // std::cout << "resulting in NPS of: " << 1.0 / (timing_temp * .000000001) << std::endl;
    //end timing

    return(best_move);
}


int* Minimax::copy_move_list(int* move_list)
{
    int* new_move_list = (int*) malloc(move_list[0] * sizeof(int));
    std::memcpy(new_move_list, move_list + 1, move_list[0] * sizeof(int));
    return(new_move_list);
}

int Minimax::decode_terminal_score(int term)
{
    if(term == 2)
    {
        return 0;
    }
    else if(term == 1)
    {
        return 1000;
    }
    else
    {
        return -1000;
    }
}

void Minimax::cleanup()
{
    //
}


//
// MONTE CARLO
//


MonteCarlo::MonteCarlo(int col, Engine* engine, std::string m_path, int sims, bool training) : Player(col, engine)
{
    model_path = m_path;

    max_sims = sims;
    curr_root = NULL;
    is_training = training;
    saved_action_probs = (float*) malloc(64 * sizeof(float));
    print_on = false;
    explore_constant = 1;
    node_storage_counter = 0;

    // data holders
    num_ints_send = 128;
    num_floats_recieve = 65;

    float_arr_reciever = (float*) malloc(num_floats_recieve * sizeof(float));

    no_decision = 0;

    temperature = 1;

    init_tensorflow();
}

void MonteCarlo::add_dirichlet_noise(float epsilon, float alpha) 
{
    auto child_cnt = curr_root->num_children;

    auto dirichlet_vector = std::vector<float>{};
    std::gamma_distribution<float> gamma(alpha, 1.0f);


    for (int i = 0; i < curr_root->num_children; i++) 
    {
        std::default_random_engine generator;
        generator.seed(rand());
        dirichlet_vector.emplace_back(gamma(generator));
    }

    auto sample_sum = std::accumulate(begin(dirichlet_vector),
                                      end(dirichlet_vector), 0.0f);

    // If the noise vector sums to 0 or a denormal, then don't try to
    // normalize.
    if (sample_sum < std::numeric_limits<float>::min()) 
    {
        return;
    }

    for (auto& v: dirichlet_vector) 
    {
        v /= sample_sum;
    }

    // printf("\ndialectric\n");
    // for(int i = 0; i < curr_root->num_children; i++)
    // {
    //     printf("%f, ", dirichlet_vector[i]);
    // }
    // printf("\n");

    int child_iter = 0;
    for (int i = 0; i < curr_root->num_children; i++) 
    {
        Node* child = curr_root->children_nodes + i;
        auto score = child->policy;
        auto eta_a = dirichlet_vector[child_iter++];
        score = score * (1 - epsilon) + epsilon * eta_a;
        // printf("orig: %f + adding: %f, ", score * (1 - epsilon), epsilon * eta_a);
        child->policy = score;
    }
    // printf("\n");
}

int MonteCarlo::move(int* move_list)
{
    // printf("color: %i\n", color);
    if(move_list[0] == 0) // passing because no moves aval
    {
        no_decision = 1;
        return -1;
    }
    if(move_list[0] == 1) // if only one choice return only move
    {
        no_decision = 1;
        return move_list[1];
    }

    no_decision = 0;

    // setting up root node info
    Node* root = create_default_node();
    root->color = color;
    root->board_hash = e->hash_board();
    // node_storage.insert({root->board_hash, root});
    node_storage.insert({node_storage_counter, root});
    node_storage_counter++;

    expand_node(root, move_list);
    curr_root = root;
    curr_root->visits++;

    // ADD NOISE
    // if(is_training)
    // {
    //     // auto alpha = 0.03f * 361.0f / BOARD_SQUARES;
    //     auto alpha = 0.5f;
    //     add_dirichlet_noise
    // }

    float alpha = .2;
    add_dirichlet_noise(0.25f, alpha);

    // printf("value of root%f\n", curr_root->value);
    // printf("\nChildren of root\n");
    // for(int i = 0; i < curr_root->num_children; i++)
    // {
    //     Node* child = curr_root->children_nodes + i;
    //     printf("child %d, policy: %f, ", i, child->policy);
    // }
    // printf("\n");


    int curr_sim = 0;
    int p_color = color;
    while(curr_sim < max_sims-1)
    {
        if(print_on) printf("\n--------- NEW SIMULATION #%i ----------\n\n", curr_sim);
        Node* leaf = traverse_tree(root, p_color); // get latest unvisited node based on previous PUCT ratings
        expand_node(leaf);
        backup_stats(leaf); // backpropagate action value of expanded node
        curr_sim++;
    }

    calc_action_probs(root);
    saved_value = root->value;

    // printf("\nroot info\n");
    // printf("my board:    %llu\n", e->get_color(color));
    // printf("enemy board: %llu\n", e->get_color(1-color));
    // printf("\nvisits: %d\ncalced_q: %f\npolicy: %f\nvalue: %f\ntotal_action_value: %f\n", curr_root->visits, curr_root->calced_q, curr_root->policy, curr_root->value, curr_root->total_action_value);

    // printf("\nchild info\n");
    // for(int i = 0; i < curr_root->num_children; i++)
    // {
    //     Node* child = curr_root->children_nodes + i;
    //     printf("\nchild %d\nvisits:             %d\ncalced_q:           %f\npolicy:             %f\nvalue:             %f\ntotal_action_value: %f\nmove: %i\n", i, child->visits, child->calced_q, child->policy, child->value, child->total_action_value, child->move);
    // }
    // printf("\n");

    // printf("saved_action_probs\n")

    // exit(0);

    Node* best_node = max_child_visits(root);

    if(print_on) printf("BEST NODE FOUND OFF OF ROOT\n");
    if(print_on) print_node_info(best_node);
    if(print_on) printf("\n");

    if(print_on) print_best_graph(root);
    if(print_on) print_all_subnodes(root);

    if(print_on) printf("deleting all nodes saved\n");

    for (std::pair<U64, Node*> element : node_storage)
    {
        free(element.second);
    }
    node_storage.clear();

    return best_node->move; // returns best immediate child node
}

Node* MonteCarlo::traverse_tree(Node* node, int p_color)
{
    if(print_on) printf("---TRAVERSAL---:\n");

    while(node->expanded)
    {
        Node* new_node = max_child_puct(node);
        push_move_wrapper(new_node->move, 
                    p_color);
        node = new_node;
        p_color = 1 - p_color;
    }

    // printf("\nchild info\n");
    // for(int i = 0; i < curr_root->num_children; i++)
    // {
    //     Node* child = curr_root->children_nodes + i;
    //     printf("\nchild %d\nvisits:             %d\ncalced_q:           %f\npolicy:             %f\nvalue:             %f\ntotal_action_value: %f\n", i, child->visits, child->calced_q, child->policy, child->value, child->total_action_value);
    // }
    // printf("\n");

    // printf("landed on:\n");
    // print_node_info(node);
    // exit(0);
    if(print_on) printf("landed on:\n");
    if(print_on) print_node_info(node);
    return node;
}

void MonteCarlo::expand_node(Node* node)
{
    int* move_list = generate_moves_wrapper(node->color);
    expand_node(node, move_list);
}

void MonteCarlo::expand_node(Node* node, int* move_list)
{
    if(print_on) printf("---EXPANSION---:\n");
    if(print_on) printf("PRE:\n");
    if(print_on) print_node_info(node);

    if(move_list[0] == 0)
    {
        int term = e->is_terminal(move_list, node->color);
        
        if(term)
        {
            // initializing a termial node            
            node->is_terminal = true;
            node->value = get_true_result();
            if(print_on) printf("POST:\n");
            if(print_on) print_node_info(node);
            return;
        }


        // initializing a PASS node
        // run network here to get policies for children, and value (needed for next line)
        calculate_value_and_policies(node->color);
        node->value = float_arr_reciever[64];
        // TEMP
        // node->value = color_multiplier(node->color) * temp_value_calc();
        // node->value = temp_value_calc();
        // TEMP

        Node* new_node = create_default_node();
        new_node->board_hash = e->hash_board();
        new_node->color = 1 - node->color;
        new_node->parent_node = node;
        new_node->is_pass = true;
        new_node->move = -1;
        new_node->policy = 1;
        // node_storage.insert({new_node->board_hash, new_node});
        node_storage.insert({node_storage_counter, new_node});
        node_storage_counter++;
        
        if(print_on) printf("POST:\n");
        if(print_on) print_node_info(node);
        return;
    }

    // run network here to get policies for children, and value (needed for next line)
    // float_arr_reciever contains 64 policies, and the 65th is the value prediciton
    calculate_value_and_policies(node->color);
    node->value = float_arr_reciever[64];

    // for(int i = 0; i < 65; i++)
    // {
    //     printf("%f, ", float_arr_reciever[i]);
    // }
    // printf("\n");


    // TEMP
    // node->value = color_multiplier(node->color) * temp_value_calc();
    // node->value = temp_value_calc();
    // TEMP

    node->children_nodes = (Node*) malloc(move_list[0] * sizeof(Node));


    //////////////// THIS CODE KINDA MAKES NO SENSE FOR NODE STORAGE, JUST A FIX FOR FREEING
    node_storage.insert({node_storage_counter, node->children_nodes});
    node_storage_counter++;
    ////////////////    


    node->num_children = move_list[0];
    for(int i = 0; i < move_list[0]; i++)
    {
        push_move_wrapper(move_list[i+1], node->color);

        // check if node is in node_storage here

        Node* new_node = node->children_nodes + i;
        init_default_node(new_node);
        new_node->board_hash = e->hash_board();
        new_node->color = 1 - node->color;
        new_node->parent_node = node;
        new_node->move = move_list[i+1];
        // CHECK IF move_list[i+1] IS THE PROPER INDEX
        new_node->policy = float_arr_reciever[move_list[i+1]]; // need network to run before this
        // node_storage.insert({new_node->board_hash, new_node});
        e->pop_move();
    }
    node->expanded = true;

    if(print_on) printf("POST:\n");
    if(print_on) print_node_info(node);
}

// backs up all statistics and places board state back to root node
void MonteCarlo::backup_stats(Node* node)
{
    float newer_value = node->value;
    int inverter = 1;
    while(node != curr_root)
    {
        node->visits++;
        node->total_action_value += (newer_value * inverter);
        node->calced_q = node->total_action_value / node->visits;
        node = node->parent_node;
        e->pop_move();
        inverter = inverter * -1;
    }

    curr_root->visits++;
    curr_root->total_action_value += (newer_value * inverter);
    curr_root->calced_q = curr_root->total_action_value / curr_root->visits;
}

// Assumes node->num_children is not null
// For black (color=0) nodes, it will find the minimum
Node* MonteCarlo::max_child_puct(Node* node)
{
    Node* best_node = node->children_nodes;
    // printf("child: %i, ", 0);
    float best_score = compute_puct(best_node);
    for(int i = 1; i < node->num_children; i++)
    {
        // printf("child: %i, ", 0);
        float temp_uct = compute_puct(node->children_nodes + i);
        if(temp_uct > best_score)
        {
            best_node = node->children_nodes + i;
            best_score = temp_uct;
        }
    }
    return best_node;
}

Node* MonteCarlo::max_child_visits(Node* node)
{
    Node* best_node = node->children_nodes;
    int best_score = best_node->visits;
    for(int i = 1; i < node->num_children; i++)
    {
        int temp_uct = (node->children_nodes + i)->visits;
        if(temp_uct > best_score)
        {
            best_node = node->children_nodes + i;
            best_score = temp_uct;
        }
    }
    return best_node;
}

float MonteCarlo::compute_puct(Node* node)
{
    float u_val = explore_constant * node->policy * 
            sqrt(node->parent_node->visits);
    u_val /= (1 + node->visits);
    // printf("PUCT: %f, uval:%f\n", (color_multiplier(node->parent_node->color) * node->calced_q) + u_val, u_val);
    return (color_multiplier(node->parent_node->color) * node->calced_q) + u_val;
}

// assumes 1 node exists
int MonteCarlo::node_argmax(Node* node, int num_nodes)
{
    int best_index = 0;
    int best_score = (node + best_index)->visits;
    for(int i = 1; i < num_nodes; i++)
    {
        int num_visits = (node + i)->visits;
        if(num_visits > best_score)
        {
            best_index = i;
            best_score = num_visits;
        }
    }
    return best_index;
}

void MonteCarlo::calc_action_probs(Node* node)
{
    memset(saved_action_probs, 0, 64 * sizeof(float));

    if(temperature == 0)
    {
        int best_index = node_argmax(node->children_nodes, node->num_children);
        saved_action_probs[node->children_nodes[best_index].move] = 1;
        return;
    }

    float sum = 0;
    Node* child;
    for(int i = 0; i < node->num_children; i++)
    {
        child = node->children_nodes + i;
        float calculation = pow(child->visits, 1.0 / temperature);
        sum += calculation;
        saved_action_probs[child->move] = calculation;
    }

    for(int i = 0; i < node->num_children; i++)
    {
        child = node->children_nodes + i;
        saved_action_probs[child->move] = saved_action_probs[child->move] / sum;
    }
}

void MonteCarlo::cleanup()
{
    printf("begining MonteCarlo cleanup\n");
    free(float_arr_reciever);
    printf("finished MonteCarlo cleanup\n");
}

void MonteCarlo::load_board_state_into_tensor(int p_color, tensorflow::Tensor &input_tensor)
{
    U64 curr_state_color = e->get_color(p_color);
    U64 opp_state_color = e->get_color(1 - p_color);
    int index;

    // tensorflow::Matrix t_matrix = input_tensor.matrix<float>();
    auto t_matrix = input_tensor.matrix<float>();
    // tensorflow/cc/burrus/player.cpp:794:25: error: 'value_type' is not a member of 'Eigen::TensorMap<Eigen::Tensor<float, 2, 1, long int>, 16, Eigen::MakePointer>'
    // std::cout << typeid(decltype(t_matrix)::value_type).name() << std::endl;

    while(curr_state_color)
    {
        index = e->lsb_digit(curr_state_color);
        t_matrix(index) = 1;
        curr_state_color -= (1ULL << index);
    }

    while(opp_state_color)
    {
        index = e->lsb_digit(opp_state_color);
        t_matrix(index + 64) = 1;
        opp_state_color -= (1ULL << index);
    }
}

void MonteCarlo::init_tensorflow()
{
    std::cout << "begining init_tensorflow" << std::endl;
    // set up input paths
    const std::string pathToGraph = model_path + "/model.ckpt.meta";
    const std::string checkpointPath = model_path + "/model.ckpt";

    tensorflow_session = tensorflow::NewSession(tensorflow::SessionOptions());
    if (tensorflow_session == nullptr) 
    {
        throw std::runtime_error("Could not create Tensorflow session.");
    }

    tensorflow::Status status;

    // Read in the protobuf graph we exported
    tensorflow::MetaGraphDef graph_def;
    status = tensorflow::ReadBinaryProto(tensorflow::Env::Default(), pathToGraph, &graph_def);
    if (!status.ok()) 
    {
        throw std::runtime_error("Error reading graph definition from " + pathToGraph + ": " + status.ToString());
    }

    // Add the graph to the session
    status = tensorflow_session->Create(graph_def.graph_def());
    if (!status.ok()) 
    {
        throw std::runtime_error("Error creating graph: " + status.ToString());
    }

    // Read weights from the saved checkpoint
    tensorflow::Tensor checkpointPathTensor(tensorflow::DT_STRING, tensorflow::TensorShape());
    checkpointPathTensor.scalar<std::string>()() = checkpointPath;
    status = tensorflow_session->Run(
            {{ graph_def.saver_def().filename_tensor_name(), checkpointPathTensor },},
            {},
            {graph_def.saver_def().restore_op_name()},
            nullptr);
    if (!status.ok()) 
    {
        throw std::runtime_error("Error loading checkpoint from " + checkpointPath + ": " + status.ToString());
    }
    std::cout << "finished init_tensorflow" << std::endl;
}

std::vector<int> MonteCarlo::get_tensor_shape(tensorflow::Tensor& tensor)
{
    std::vector<int> shape;
    int num_dimensions = tensor.shape().dims();
    for (int ii_dim = 0; ii_dim < num_dimensions; ii_dim++) 
    {
        shape.push_back(tensor.shape().dim_size(ii_dim));
    }
    return shape;
}

// working with eigen tensor
// input_tensor: http://eigen.tuxfamily.org/index.php?title=Tensor_support#Using_the_Tensor_module
void MonteCarlo::tensorflow_pass(int p_color)
{
    // ###### PROCESS DATA VIA FORWARD PASS
    // x_data = [inters]
    // res = sess.run([policy_head_output, value_head_output], feed_dict={x_tensor: x_data, train_bool: False})
    // policy_calced = res[0][0]
    // value_calced = res[1][0]
    // together = np.append(policy_calced, value_calced)
    // // ######

    std::cout << "begining tensorflow pass" << std::endl;

    tensorflow::Status status;

    tensorflow::Tensor input_tensor(tensorflow::DT_FLOAT, tensorflow::TensorShape({1, 128}));
    tensorflow::Tensor train_bool(tensorflow::DT_BOOL, tensorflow::TensorShape());
    train_bool.scalar<bool>()() = false;
    load_board_state_into_tensor(p_color, input_tensor);

    std::cout << "Loaded board state in" << std::endl;

    std::vector<std::pair<tensorflow::string, tensorflow::Tensor>> feedDict = { {"x:0", input_tensor}, {"train_bool", train_bool} };
    std::vector<tensorflow::Tensor> outputTensors;
    
    std::cout << "starting Run" << std::endl;
    status = tensorflow_session->Run(feedDict, { "value_head_output:0", "policy_head_output/BiasAdd:0" }, {}, &outputTensors);
    std::cout << "finished Run" << std::endl;

    if (!status.ok())
    {
        throw std::runtime_error("Error evaluating run: " + status.ToString());
    }

    tensorflow::Tensor value_head_output = outputTensors[0];
    tensorflow::Tensor policy_head_output = outputTensors[1];


    std::cout << "value_head_output: " << value_head_output.scalar<float>() << std::endl;
    
    std::vector<int> policy_head_shape = get_tensor_shape(policy_head_output);
    std::cout << "The shape here" << std::endl;
    for (int i = 0; i < policy_head_shape.size(); i++)
    {
        std::cout << policy_head_shape[i] << std::endl;
    }

    auto to_vector_form = policy_head_output.flat<float>();
    std::cout << "policy_head_output" << std::endl;
    for (int i = 0; i < 64; i++)
    {
        std::cout << to_vector_form(i) << std::endl;
    }


    auto to_vector_form = input_tensor.flat<float>();
    std::cout << "input_tensor" << std::endl;
    for (int i = 0; i < 128; i++)
    {
        std::cout << to_vector_form(i) << std::endl;
    }

    
    // image_tensor.vec<int8>()(i)

    // auto t_matrix2 = outputTensors.matrix<float>();
    // std::cout << outputTensors << std::endl;

    std::cout << "finished tensorflow_pass" << std::endl;
    exit(0);
}

// at the end the proper values will be loaded into float_arr_reciever
void MonteCarlo::calculate_value_and_policies(int p_color)
{
    tensorflow_pass(p_color);
    // memcpy(float_arr_reciever, pSharedMemory_rest, num_floats_recieve * sizeof(float));
}

//
// helper functions
//

// returns 1 for white and -1 for black
int MonteCarlo::color_multiplier(int p_color)
{
    return (p_color * 2) - 1;
}

int MonteCarlo::get_true_result()
{
    int temp = e->score_board();
    if(temp > 0)
    {
        return 1;
    }
    else if(temp < 0)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

void MonteCarlo::init_default_node(Node* node)
{
    node->board_hash = e->hash_board();
    node->parent_node = NULL;
    node->children_nodes = NULL;
    node->num_children = 0;

    node->expanded = false;
    node->is_terminal = false;
    node->is_pass = false;
    node->color = -1;
    node->move = -1;
    node->visits = 0;    
    node->total_action_value = 0;
    node->calced_q = 0;    
    node->policy = 0;
    node->value = 0;
}

Node* MonteCarlo::create_default_node()
{
    // Node* node_pointer = new Node;
    Node* node_pointer = (Node*) malloc(sizeof(Node));
    init_default_node(node_pointer);
    return node_pointer;
}

int* MonteCarlo::generate_moves_wrapper(int p_color)
{
    if(p_color)
    {
        return e->generate_white_moves();
    }
    else
    {
        return e->generate_black_moves();
    }
}

void MonteCarlo::push_move_wrapper(int move, int p_color)
{
    if(p_color)
    {
        return e->push_white_move(move);
    }
    else
    {
        return e->push_black_move(move);
    }
}

void MonteCarlo::print_node_info(Node* node)
{
    std::cout << "board_hash: " << node->board_hash << 
            ", self_pointer: " << (void*) node << 
            ", parent_node_pointer: " << (void*) node->parent_node << 
            ", num_children: " << node->num_children << 
            ", expanded: " << node->expanded <<
            ", is_pass: " << node->is_pass << 
            ", is_terminal: " << node->is_terminal <<
            ", color: " << node->color << 
            ", visits: " << node->visits << 
            ", value: " << node->value <<
            ", policy: " << node->policy <<
            ", calced_q: " << node->calced_q << "\n";
}

void MonteCarlo::print_best_graph(Node* node)
{
    printf("\n\nPRINTING GRAPH OF NODES OF ALL HIGHEST VISIT COUNTS\n\n");
    int depth = 0;
    // printf("DEPTH OF: %i\n", depth);
    // print_node_info(node);
    while(node->expanded)
    {
        printf("DEPTH OF: %i, ", depth);
        print_node_info(node);
        Node* best_child = max_child_visits(node);
        node = best_child;
        depth++;
    }
}

void MonteCarlo::print_all_subnodes(Node* node)
{
    printf("\n\nPRINTING ALL NODES IN GRAPH\n\n");
    print_all_subnodes_helper(node, 0);
}

void MonteCarlo::print_all_subnodes_helper(Node* node, int depth)
{
    printf("DEPTH OF: %i, ", depth);
    print_node_info(node);
    for(int i = 0; i < node->num_children; i++)
    {
        if((node->children_nodes+i)->expanded) print_all_subnodes_helper(node->children_nodes+i, depth+1);
        // print_all_subnodes_helper(node->children_nodes+i, depth+1);
    }
}

// saver functions

// always will be 64 in length
float* MonteCarlo::get_saved_action_probs()
{
    return saved_action_probs;
}

// the value return
float MonteCarlo::get_saved_value()
{
    return saved_value;
}