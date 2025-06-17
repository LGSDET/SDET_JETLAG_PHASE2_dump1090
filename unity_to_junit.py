import sys
import re
from xml.etree.ElementTree import Element, SubElement, ElementTree

def remove_ansi_escape(s):
    ansi_escape = re.compile(r'\x1B[@-_][0-?]*[ -/]*[@-~]')
    return ansi_escape.sub('', s)

if len(sys.argv) != 3:
    print("Usage: unity_to_junit.py test_output.txt test_result.xml")
    sys.exit(2)

input_path = sys.argv[1]
output_path = sys.argv[2]

with open(input_path, encoding='utf-8') as f:
    lines = [remove_ansi_escape(line.strip()) for line in f.readlines()]

testsuite = Element('testsuite')
testsuite.set('name', 'UnityTests')
testsuite.set('tests', '0')
testsuite.set('failures', '0')

total = 0
failures = 0

for line in lines:
    m = re.match(r'(.+?):(\d+):(.+?):(PASS|FAIL)(:.*?)?$', line)
    if m:
        file, line_no, name, result, msg = m.groups()
        testcase = SubElement(testsuite, 'testcase')
        testcase.set('classname', file)
        testcase.set('name', name)
        testcase.set('line', line_no)
        total += 1

        if result == 'FAIL':
            failures += 1
            failure = SubElement(testcase, 'failure')
            failure.text = (msg or '').strip(': ')

testsuite.set('tests', str(total))
testsuite.set('failures', str(failures))

tree = ElementTree(testsuite)
tree.write(output_path, encoding='utf-8', xml_declaration=True)
