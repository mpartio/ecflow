#!/usr/bin/env python3

import requests
import inspect
import sys
import pytest
import time
import base64
from requests.auth import HTTPBasicAuth
from requests.packages import urllib3

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

USER = 'partio'
PASSWORD = 'partio'
ECFLOW_HOST = 'ecflow-server'
ECFLOW_PORT = 3141

API_URL = 'https://localhost:8080'
AUTH_URL = 'http://localhost:5001'

SUITE_NAME = '/test'
SUITE_FILE_PATH = '../../../ecflow-devel/image/'
SUITE_DEFINITION_FILE = 'test.def'.format(SUITE_FILE_PATH)

def load_file(filename):
    filename = '{}/{}'.format(SUITE_FILE_PATH, filename)

    with open(filename) as fp:
        content = fp.read()
        print(content)
        return requests.utils.quote(base64.b64encode(content.encode('utf-8')).decode())

def get_suite_definition(filename):
   return load_file(filename)

def handle_response(response, expected_code = [200]):
    print(inspect.stack()[1][3])
    print(response.url)
    print('Return code: {}'.format(response.status_code))
    print('Return value: {}'.format(response.content))
    assert(response.status_code in expected_code)

    return response

def request(method, url, verify=False, auth=(USER, PASSWORD)):
    return requests.request(method, url=url, verify=verify, auth=auth)

def get_url():
    return '{}/query?host={}&port={}'.format(API_URL, ECFLOW_HOST, ECFLOW_PORT)

@pytest.mark.dependency()
def test_ping():
    handle_response(request('get', '{}&command=ping'.format(get_url())))
    
def test_version():
    handle_response(request('get', '{}&command=version'.format(get_url())))

def test_clean_suite():
    handle_response(request('delete', '{}&command=delete&argument=yes&argument2={}'.format(get_url(), SUITE_NAME)), expected_code = [200,404])

@pytest.mark.dependency(depends=["test_ping"])
def test_load_suite():
    handle_response(request('post', '{}&command=load&argument={}'.format(get_url(), get_suite_definition(SUITE_DEFINITION_FILE))))

@pytest.mark.dependency(depends=["test_load_suite"])
def test_suites():
    resp = handle_response(request('get', '{}&command=suites'.format(get_url())))

    assert(resp.content.decode('utf-8').strip() == 'test')

@pytest.mark.skip(reason="pakkosubmittaa taskin")
@pytest.mark.dependency(depends=["test_load_suite"])
def test_edit_script():
    handle_response(request('put', '{}&command=edit_script&argument1=/test/a/b&argument2=submit_file&argument3={}&argument4=false&argument5=no_run'.format(get_url(), load_file('a.ecf'))))
    handle_response(request('put', '{}&command=edit_script&argument1=/test/a/b&argument2=edit'.format(get_url())))

@pytest.mark.dependency(depends=["test_load_suite"])
def test_begin_suite():
    handle_response(request('put', '{}&command=begin&argument={}'.format(get_url(), SUITE_NAME)))

@pytest.mark.dependency(depends=["test_load_suite"])
def test_get_suite():
    response = handle_response(request('get', '{}&command=get&argument={}'.format(get_url(), SUITE_NAME)))

    correct = b"#5.8.3\nsuite test\n  edit ECF_FILES '/tmp'\n  edit ECF_INCLUDE '/tmp'\n  edit ECF_TRIES '1'\n  family a\n    event begin\n    task a\n      trigger ../a:begin\n    task b\n      trigger a eq complete\n      meter progress 1 100 100\n      event done\n    task c\n      trigger b == active\n  endfamily\n  family b\n    event begin\n    task a_api\n      trigger ../b:begin\n    task b_api\n      trigger a_api eq complete\n      meter progress 1 100 100\n      event done\n    task c_api\n      trigger b_api == active\n  endfamily\nendsuite\n# enddef\n"

    assert(response.content == correct)

@pytest.mark.dependency(depends=['test_begin_suite'])
def test_add():
    handle_response(request('post', '{}&command=add&argument1=foo&argument2=bar'.format(get_url())), expected_code=[405])

@pytest.mark.dependency(depends=['test_begin_suite'])
def test_add_variable():
    add_variable('/test', 'foo', 'bar')

@pytest.mark.dependency(depends=['test_add_variable'])
def test_query_variable():
    resp = handle_response(request('get', '{}&command=query&argument1=variable&argument2=/test:foo'.format(get_url())))
    assert(resp.content.decode('utf-8').strip() == "bar")

def add_variable(node, key, value):
    handle_response(request('put', '{}&command=alter&argument1=add&argument2=variable&argument3={}&argument4={}&argument5={}'.format(get_url(), key, value, node)))

def node_op(node, op):
    handle_response(request('put', '{}&command={}&argument1={}'.format(get_url(), op, node)))

@pytest.mark.dependency(depends=['test_begin_suite'])
def test_requeue():
    node_op('/test', 'requeue')

@pytest.mark.dependency(depends=["test_begin_suite"])
def test_suspend_suite():
    handle_response(request('put', '{}&command=suspend&argument1=/test'.format(get_url())))

@pytest.mark.dependency(depends=["test_suspend_suite"])
def test_resume_suite():
    handle_response(request('put', '{}&command=resume&argument1=/test'.format(get_url())))

@pytest.mark.dependency(depends=['test_begin_suite']) #,'test_edit_script'])
def test_start_family_a():
    node_op("/test/a", "requeue")

    handle_response(request('put', '{}&command=alter&argument1=change&argument2=event&argument3=begin&argument4=set&argument5=/test/a'.format(get_url())))

    for task in ['a', 'b', 'c']:
        status = None
        cnt=0
        while status != "complete":
            status = request('get', '{}&command=query&argument1=state&argument2=/test/a/{}'.format(get_url(), task)).content.decode("utf-8").strip()
            print("Task {} status {}".format(task, status))
            if status != "complete":
                cnt += 1
                time.sleep(2)
                if cnt > 20:
                    sys.exit(1)

@pytest.mark.dependency(depends=['test_begin_suite'])# ,'test_edit_script'])
def test_start_family_b():
    node_op('/test/b', 'requeue')

    add_variable('/test/b', 'ECF_API_HOST', 'ecflow-devel')

    handle_response(request('put', '{}&command=alter&argument1=change&argument2=event&argument3=begin&argument4=set&argument5=/test/b'.format(get_url())))

    for task in ['a_api', 'b_api', 'c_api']:
        status = None
        cnt=0
        while status != "complete":
            status = request('get', '{}&command=query&argument1=state&argument2=/test/b/{}'.format(get_url(), task)).content.decode("utf-8").strip()
            print("Task {} status {}".format(task, status))
            if status != "complete":
                cnt += 1
                time.sleep(2)
                if cnt > 20:
                    sys.exit(1)

@pytest.mark.dependency(depends=["test_load_suite"])
def test_group():
    handle_response(request('get', '{}&command=group&argument=get;why%20/test'.format(get_url())))
    handle_response(request('get', '{}&command=group&argument={}'.format(get_url(), requests.utils.quote('halt=yes;reloadpasswdfile;restart;'))))

@pytest.mark.dependency(depends=["test_load_suite","test_start_family_b"])
def test_delete_suite():
    handle_response(request('delete', '{}&command=delete&argument=yes&argument2={}'.format(get_url(), SUITE_NAME)))

def test_user():
    handle_response(request('get', '{}&command=user&argument=fred'.format(get_url())), expected_code=[405])

def test_unknown():
    handle_response(request('get' ,'{}&command=xxxyyy'.format(get_url())), expected_code = [500])

def test_server_version():
    handle_response(request('get' ,'{}&command=server_version'.format(get_url())))

def test_ssl():
    handle_response(request('get' ,'{}&command=stats&ssl=on'.format(get_url())))

def test_stats():
    handle_response(request('get' ,'{}&command=stats'.format(get_url())))

def test_stats_reset():
    handle_response(request('put' ,'{}&command=stats_reset'.format(get_url())))

def test_stats_server():
    handle_response(request('get' ,'{}&command=stats_server'.format(get_url())))

@pytest.mark.skip(reason="no way of currently testing this")
def test_terminate():
    handle_response(request('put' ,'{}&command=terminate&argument=yes'.format(get_url())))

@pytest.mark.skip(reason="no way of currently testing this")
def test_token_authentication():
    response = handle_response(requests.post('{}/auth'.format(AUTH_URL), verify=False, json = { 'username' : 'fred', 'password' : 'frogs' }))

    token = response.json()['access_token']

    response = handle_response(requests.put('{}/query?command=log&argument=get&argument2=5'.format(URL), verify=False, headers = {'Authorization' : 'Bearer {}'.format(token) }))

def suite_operations():
    test_clean_suite()
 
    test_load_suite()
    test_begin_suite()
    test_get_suite()
    test_edit_script()
    test_delete_suite()
 

def main():
    if len(sys.argv) > 1:
        cmd = sys.argv[1] + '('
        for arg in sys.argv[2:]:
            cmd += '"{}"'.format(arg)
        if cmd[-1] == ',':
            cmd = cmd[-2]
        cmd += ')'
        print(cmd)
        eval(cmd)
        sys.exit(0)

    test_ping()
    test_version()
    test_user()
    test_unknown()
#    test_token_authentication()
    suite_operations()


if __name__ == '__main__':
    main()
