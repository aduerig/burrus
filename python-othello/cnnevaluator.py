import tensorflow as tf
import numpy as np

import os

os.environ['TF_CPP_MIN_LOG_LEVEL'] = '3' 

class CnnEvaluator:
    def __init__(self):
        self.sess = tf.Session()
        model_dir, self.model_num = latest_model_dir()
        saver = tf.train.import_meta_graph(os.path.join(model_dir, 'model.ckpt.meta'))
        saver.restore(self.sess, os.path.join(model_dir, 'model.ckpt'))
        self.graph=tf.get_default_graph()


    def evaluate_board(self, e, board):
        flat = e.flat_board(board)
        x_data = [flat]
        x_tensor = self.graph.get_tensor_by_name('x:0')

        value_head_output = self.graph.get_tensor_by_name('value_head_output:0')
        policy_head_output = self.graph.get_tensor_by_name('policy_head_output/BiasAdd:0')

        res = self.sess.run([policy_head_output, value_head_output], feed_dict={x_tensor: x_data})
        return res







# just for testing atm
def main():

    with tf.Session() as sess:
        model_dir = latest_model_dir()
        print('Using model at: ' + model_dir)

        saver = tf.train.import_meta_graph(os.path.join(model_dir, 'model.ckpt.meta'))
        saver.restore(sess, os.path.join(model_dir, 'model.ckpt'))

        graph=tf.get_default_graph()

        x_data = np.array([np.arange(0, 128, dtype=np.float32)])
        x_tensor = graph.get_tensor_by_name('x:0')

        value_head_output = graph.get_tensor_by_name('value_head_output:0')
        policy_head_output = graph.get_tensor_by_name('policy_head_output/BiasAdd:0')

        res = sess.run([policy_head_output, value_head_output], feed_dict={x_tensor: x_data})
        print(res)












def latest_model_dir():
    data_dir_name = 'data'
    data_dir = os.path.abspath(os.path.join(os.getcwd(), os.pardir, data_dir_name))
    newest_model = len(next(os.walk(data_dir))[1])-1
    return os.path.join(data_dir, 'model_' + str(newest_model)), newest_model



if __name__ == "__main__":
    main()