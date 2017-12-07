from setuptools import setup,find_packages

setup(name='my-me',
    version='0.1.0',
    packages=find_packages(),
    install_requires=['numpy>=1.13','me-data>=0.1'],
    package_data={
    '':['*.so', '*.py']
    },
    author='MYCAPITAL',
    author_email='rss@mycapital.net',
    description='mycapital.net match engine interfaces',
    url='https://www.mycapital.net'
)

