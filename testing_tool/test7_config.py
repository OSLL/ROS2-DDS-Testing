import os
from general_funcs import mkdir_config, constr_resfilename, constr_config


def test7_config():
    configs = []
    mkdir_config(7)
    args = {"topic":['test_topic', 'test_topic1'], 
            "res_filenames":['pub', 'sub'],
            "m_count":10000, "min_msg_size":128-256*1024, "max_msg_size":2*1024*1024,
            "step":256*1024, "msgs_before_step":100,
            "priority":[-1, -1], "cpu_index":[-1, -1], 
            "interval":0, "topic_priority":100}
    pair_count = [1, 5, 20, 25]
    for p in pair_count:
        try:
            os.mkdir('test_7/config/' + str(p))
        except OSError:
            None
        for i in range(0, p):
            name = str(p) + '/' + str(i)
            args["topic"][0] = 'test_topic' + str(2*i)
            args["topic"][1] = 'test_topic' + str(2*i+1)
            args["res_filenames"][0] = constr_resfilename(name, 'p')
            args["res_filenames"][1] = constr_resfilename(name, 's')
            configs.append(constr_config(7, name, args))
    return configs
